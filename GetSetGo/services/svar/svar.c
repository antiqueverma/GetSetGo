
#include "gsg_config.h"
#if (defined(GSG_USE_SVAR) && (GSG_USE_SVAR == GSG_ENABLE))
#include <ctype.h>   // for isprint
#include <string.h>  // for strlen
#include "services/debug/debug.h"
#include "svar.h"
#include "sys/nvm/nvm.h"

// Pointers list
static svar_module_t *svarModulesRegistry[SVAR_MAX_MODULES];

#if (GSG_OS_USED == GSG_OS_FREERTOS)
static SemaphoreHandle_t svarMutex = NULL;
#endif

static void _svarInitMutex(void)
{
#if (GSG_OS_USED == GSG_OS_FREERTOS)
    if (svarMutex == NULL)
    {
        svarMutex = xSemaphoreCreateMutex();
    }
#endif
}

static bool _svarGetLock(void)
{
#if (GSG_OS_USED == GSG_OS_FREERTOS)
    _svarInitMutex();
    return (svarMutex != NULL) && (xSemaphoreTake(svarMutex, portMAX_DELAY) == pdTRUE);
#else
    return true;
#endif
}

static bool _svarReleaseLock(void)
{
#if (GSG_OS_USED == GSG_OS_FREERTOS)
    if (svarMutex == NULL)
    {
        return false;
    }
    return xSemaphoreGive(svarMutex) == pdTRUE;
#else
    return true;
#endif
}

static int _validate(system_variable_t *sv, svar_value_t *val)
{
    if (sv == NULL || val == NULL)
    {
        return FAIL;
    }

    switch (sv->type)
    {
        case SVAR_TYPE_INT8:
            if (val->i8 < sv->min.i8 || val->i8 > sv->max.i8) return FAIL;
            break;
        case SVAR_TYPE_UINT8:
            if (val->u8 < sv->min.u8 || val->u8 > sv->max.u8) return FAIL;
            break;
        case SVAR_TYPE_INT16:
            if (val->i16 < sv->min.i16 || val->i16 > sv->max.i16) return FAIL;
            break;
        case SVAR_TYPE_UINT16:
            if (val->u16 < sv->min.u16 || val->u16 > sv->max.u16) return FAIL;
            break;
        case SVAR_TYPE_INT32:
            if (val->i32 < sv->min.i32 || val->i32 > sv->max.i32) return FAIL;
            break;
        case SVAR_TYPE_UINT32:
            if (val->u32 < sv->min.u32 || val->u32 > sv->max.u32) return FAIL;
            break;
        #if SVAR_ENABLE_64_BIT
        case SVAR_TYPE_INT64:
            if (val->i64 < sv->min.i64 || val->i64 > sv->max.i64) return FAIL;
            break;
        case SVAR_TYPE_UINT64:
            if (val->u64 < sv->min.u64 || val->u64 > sv->max.u64) return FAIL;
            break;
        #endif
        case SVAR_TYPE_FLOAT:
            if (val->f < sv->min.f || val->f > sv->max.f) return FAIL;
            break;

        case SVAR_TYPE_BOOL:
            if (!(val->b == 0 || val->b == 1)) return FAIL;
            break;

        case SVAR_TYPE_CHAR:
            if (!isprint((unsigned char)val->c)) return FAIL;
            break;

        case SVAR_TYPE_STRING:
        {
            if (val->str == NULL)
            {
                return FAIL;
            }

            uint16_t max_len = sv->max.u16;

            // length check
            // if (strlen(val->str) >= max_len)
            // {
            //     return FAIL;
            // }

            // printable check
            for (uint16_t i = 0; val->str[i] != '\0'; i++)
            {
                if (!isprint((unsigned char)val->str[i]))
                {
                    return FAIL;
                }
            }
            break;
        }

        default:
            return FAIL;
    }

    return PASS;
}

static void _genericSetCb(system_variable_t *sv)
{
    (void)sv;
    // future hooks
}

static uint16_t _getTypeSize(svar_type_t type, system_variable_t *sv)
{
    switch (type)
    {
        case SVAR_TYPE_INT8:
        case SVAR_TYPE_UINT8:
        case SVAR_TYPE_BOOL:
        case SVAR_TYPE_CHAR:
            return 1;
        case SVAR_TYPE_INT16:
        case SVAR_TYPE_UINT16:
            return 2;
        case SVAR_TYPE_INT32:
        case SVAR_TYPE_UINT32:
        case SVAR_TYPE_FLOAT:
            return 4;
    #if SVAR_ENABLE_64_BIT
        case SVAR_TYPE_INT64:
        case SVAR_TYPE_UINT64:
            return 8;
    #endif
        case SVAR_TYPE_STRING:
            return sv->max.u16;  // buffer length
        default:
            return 0;
    }
}

static void _nvmWrite(svar_module_t *module, system_variable_t *sv)
{
    if (sv == NULL || sv->flags.persistent == 0)
        return;

    uint16_t size = _getTypeSize(sv->type, sv);

    if (sv->category != SVAR_CAT_RUNTIME)
    {
        if (sv->type == SVAR_TYPE_STRING)
        {
            NVM_writeData(module->nvmHandle,
                        (nvm_address_t)sv->nvmAddr,
                        (nvm_data_t *)sv->value.str,
                        size);
        }
        else
        {
            NVM_writeData(module->nvmHandle,
                        (nvm_address_t)sv->nvmAddr,
                        (nvm_data_t *)&sv->value,
                        size);
        }
    }
    else
    {
        // runtime → handled by page flush later
    }
}

static void _nvmRead(svar_module_t *module, system_variable_t *sv)
{
    if (sv == NULL || sv->flags.persistent == 0)
        return;

    uint16_t size = _getTypeSize(sv->type, sv);

    if (sv->type == SVAR_TYPE_STRING)
    {
        NVM_readData(module->nvmHandle,
                    (nvm_address_t)sv->nvmAddr,
                    (nvm_data_t *)sv->value.str,
                    size);

        sv->value.str[size - 1] = '\0';
    }
    else
    {
        NVM_readData(module->nvmHandle,
                     (nvm_address_t)sv->nvmAddr,
                     (nvm_data_t *)&sv->value,
                     size);
    }
}

static gsg_result_t _setValue(system_variable_t *sv, svar_value_t *val)
{
    if (!_svarGetLock())
    {
        return GSG_BUSY;
    }

    if (sv == NULL || val == NULL)
    {
        _svarReleaseLock();
        return GSG_INVALID_ARG;
    }

    sv->value = *val;

    _svarReleaseLock();
    return GSG_SUCCESS;
}

static gsg_result_t _getValue(system_variable_t *sv, svar_value_t *val)
{
    if (!_svarGetLock())
    {
        return GSG_BUSY;
    }

    if (sv == NULL || val == NULL)
    {
        _svarReleaseLock();
        return GSG_INVALID_ARG;
    }

    *val = sv->value;
    _svarReleaseLock();
    return GSG_SUCCESS;
}

static gsg_result_t _setMin(system_variable_t *sv, svar_value_t *val)
{
    if (!_svarGetLock())
    {
        return GSG_BUSY;
    }

    if (sv == NULL || val == NULL)
    {
        _svarReleaseLock();
        return GSG_INVALID_ARG;
    }

    sv->min = *val;
    _svarReleaseLock();
    return GSG_SUCCESS;
}

static gsg_result_t _getMin(system_variable_t *sv, svar_value_t *val)
{
    if (!_svarGetLock())
    {
        return GSG_BUSY;
    }

    if (sv == NULL || val == NULL)
    {
        _svarReleaseLock();
        return GSG_INVALID_ARG;
    }

    *val = sv->min;
    _svarReleaseLock();
    return GSG_SUCCESS;
}

static gsg_result_t _setMax(system_variable_t *sv, svar_value_t *val)
{
    if (!_svarGetLock())
    {
        return GSG_BUSY;
    }

    if (sv == NULL || val == NULL)
    {
        _svarReleaseLock();
        return GSG_INVALID_ARG;
    }

    sv->max = *val;
    _svarReleaseLock();
    return GSG_SUCCESS;
}

static gsg_result_t _getMax(system_variable_t *sv, svar_value_t *val)
{
    if (!_svarGetLock())
    {
        return GSG_BUSY;
    }

    if (sv == NULL || val == NULL)
    {
        _svarReleaseLock();
        return GSG_INVALID_ARG;
    }

    *val = sv->max;
    _svarReleaseLock();
    return GSG_SUCCESS;
}

static gsg_result_t _setDefault(system_variable_t *sv, svar_value_t *val)
{
    if (!_svarGetLock())
    {
        return GSG_BUSY;
    }

    if (sv == NULL || val == NULL)
    {
        _svarReleaseLock();
        return GSG_INVALID_ARG;
    }

    sv->def = *val;
    _svarReleaseLock();
    return GSG_SUCCESS;
}

static gsg_result_t _getDefault(system_variable_t *sv, svar_value_t *val)
{
    if (!_svarGetLock())
    {
        return GSG_BUSY;
    }

    if (sv == NULL || val == NULL)
    {
        _svarReleaseLock();
        return GSG_INVALID_ARG;
    }

    *val = sv->def;
    _svarReleaseLock();
    return GSG_SUCCESS;
}

static void _copyToUnion(system_variable_t *sv, svar_value_t *dst, void *src)
{
    switch (sv->type)
    {
        case SVAR_TYPE_INT8:    dst->i8  = *(int8_t*)src; break;
        case SVAR_TYPE_UINT8:   dst->u8  = *(uint8_t*)src; break;
        case SVAR_TYPE_INT16:   dst->i16 = *(int16_t*)src; break;
        case SVAR_TYPE_UINT16:  dst->u16 = *(uint16_t*)src; break;
        case SVAR_TYPE_INT32:   dst->i32 = *(int32_t*)src; break;
        case SVAR_TYPE_UINT32:  dst->u32 = *(uint32_t*)src; break;
        #if SVAR_ENABLE_64_BIT
        case SVAR_TYPE_INT64:   dst->i64 = *(int64_t*)src; break;
        case SVAR_TYPE_UINT64:  dst->u64 = *(uint64_t*)src; break;
        #endif
        case SVAR_TYPE_FLOAT:   dst->f   = *(float*)src; break;
        case SVAR_TYPE_BOOL:    dst->b   = *(uint8_t*)src; break;
        case SVAR_TYPE_CHAR:    dst->c   = *(char*)src; break;
        case SVAR_TYPE_STRING:
            dst->str = (char*)src;
            break;

        default:
            break;
    }
}

static void _copyFromUnion(system_variable_t *sv, void *dst, svar_value_t *src)
{
    switch (sv->type)
    {
        case SVAR_TYPE_INT8:    *(int8_t*)dst  = src->i8; break;
        case SVAR_TYPE_UINT8:   *(uint8_t*)dst  = src->u8; break;
        case SVAR_TYPE_INT16:   *(int16_t*)dst = src->i16; break;
        case SVAR_TYPE_UINT16:  *(uint16_t*)dst = src->u16; break;
        case SVAR_TYPE_INT32:   *(int32_t*)dst = src->i32; break;
        case SVAR_TYPE_UINT32:  *(uint32_t*)dst = src->u32; break;
        #if SVAR_ENABLE_64_BIT
        case SVAR_TYPE_INT64:   *(int64_t*)dst = src->i64; break;
        case SVAR_TYPE_UINT64:  *(uint64_t*)dst = src->u64; break;
        #endif
        case SVAR_TYPE_FLOAT:   *(float*)dst = src->f; break;
        case SVAR_TYPE_BOOL:    *(bool*)dst = src->b; break;
        case SVAR_TYPE_CHAR:    *(char*)dst = src->c; break;

        case SVAR_TYPE_STRING:
        {
            if (dst && src->str)
            {
                uint16_t max = sv->max.u16;
                strncpy((char*)dst, src->str, max);
                ((char*)dst)[max - 1] = '\0';
            }
            break;
        }

        default:
            break;
    }
}


static void _initModule(svar_module_t *module)
{
    for (uint32_t i = 0; i < module->count; i++)
    {
        system_variable_t *sv = &module->table[i];

        // copy default to value
        sv->value = sv->def;

        // load from NVM if persistent
        if (sv->flags.persistent == 1)
        {
            _nvmRead(module, sv);
        }
    }
}

gsg_result_t SVAR_registerModule(svar_module_t *module)
{
    if (module == NULL || module->table == NULL || module->count == 0)
    {
        return GSG_INVALID_ARG;
    }

    _svarInitMutex();

    // Simple registration without duplicate check for demo purposes
    uint16_t i;
    for (i = 0; i < SVAR_MAX_MODULES; i++)
    {
        if (svarModulesRegistry[i] == NULL)
        {
            _svarGetLock();
            svarModulesRegistry[i] = (svar_module_t*)module;
            _initModule(module);
            _svarReleaseLock();
            break;
        }
    }
    if (i == SVAR_MAX_MODULES)
    {
        return GSG_ERROR; // No space for more modules
    }

    return GSG_SUCCESS;
}

system_variable_t *_getSvarFromId(uint32_t id)
{
    for (uint16_t i = 0; i < SVAR_MAX_MODULES; i++)
    {
        if (svarModulesRegistry[i] != NULL)
        {
            if((svarModulesRegistry[i]->varOffset <= id) && (id < svarModulesRegistry[i]->varOffset + svarModulesRegistry[i]->count))
            {
                for (uint16_t j = 0; j < svarModulesRegistry[i]->count; j++)
                {
                    if (svarModulesRegistry[i]->table[j].id == id)
                    {
                        return &svarModulesRegistry[i]->table[j];
                    }
                }
            }

        }
    }
    return NULL;
}

svar_module_t *_getModuleFromId(uint32_t id)
{
    for (uint16_t i = 0; i < SVAR_MAX_MODULES; i++)
    {
        if (svarModulesRegistry[i] != NULL)
        {
            if((svarModulesRegistry[i]->varOffset <= id) && (id < svarModulesRegistry[i]->varOffset + svarModulesRegistry[i]->count))
            {
                return svarModulesRegistry[i];
            }
        }
    }
    return NULL;
}

gsg_result_t SVAR_Set(uint32_t id, void *data)
{
    if (data == NULL)
    {
        return GSG_INVALID_ARG;
    }

    system_variable_t *sv = _getSvarFromId(id);
    if (sv == NULL)
    {
        return GSG_NOT_FOUND;
    }

    svar_module_t *module = _getModuleFromId(id);

    svar_value_t temp;
    _copyToUnion(sv, &temp, data);

    if (_validate(sv, &temp) != PASS)
    {
        return GSG_ERROR;
    }

    gsg_result_t result = _setValue(sv, &temp);
    if (result != GSG_SUCCESS)
    {
        return result;
    }

    if (sv->flags.persistent == 1)
    {
        _nvmWrite(module, sv);
    }

    if (sv->setCb != NULL)
    {
        sv->setCb(&sv->value);
    }

    _genericSetCb(sv);
    return GSG_SUCCESS;
}

gsg_result_t SVAR_Get(uint32_t id, void *data)
{
    if (data == NULL)
    {
        return GSG_INVALID_ARG;
    }

    system_variable_t *sv = _getSvarFromId(id);
    if (sv == NULL)
    {
        return GSG_NOT_FOUND;
    }

    svar_value_t temp;
    gsg_result_t result = _getValue(sv, &temp);
    if (result != GSG_SUCCESS)
    {
        return result;
    }

    if (sv->getCb != NULL)
    {
        sv->getCb(&temp);
    }

    _copyFromUnion(sv, data, &temp);
    return GSG_SUCCESS;
}

int SVAR_Reset(uint32_t id)
{
    if (!_svarGetLock())
    {
        return FAIL;
    }

    system_variable_t *sv = _getSvarFromId(id);
    svar_module_t *module = _getModuleFromId(id);

    if (sv == NULL)
    {
        _svarReleaseLock();
        return FAIL;
    }

    // copy default → value
    if (sv->type == SVAR_TYPE_STRING)
    {
        strncpy(sv->value.str, sv->def.str, sv->max.u16);
        sv->value.str[sv->max.u16 - 1] = '\0';
    }
    else
    {
        sv->value = sv->def;
    }

    _svarReleaseLock();

    if (sv->flags.persistent == 1)
    {
        _nvmWrite(module, sv);
    }

    _genericSetCb(sv);

    if (sv->setCb != NULL)
    {
        sv->setCb(&sv->value);
    }

    return PASS;
}

int SVAR_ResetModule(svar_module_t *module)
{
    if (module == NULL || module->table == NULL || module->count == 0)
    {
        return FAIL;
    }

    if (!_svarGetLock())
    {
        return FAIL;
    }

    for (uint16_t j = 0; j < module->count; j++)
    {
        system_variable_t *sv = &module->table[j];

        if (sv->type == SVAR_TYPE_STRING)
        {
            strncpy(sv->value.str, sv->def.str, sv->max.u16);
            sv->value.str[sv->max.u16 - 1] = '\0';
        }
        else
        {
            sv->value = sv->def;
        }

        _genericSetCb(sv);

        if (sv->flags.persistent == 1)
        {
            _nvmWrite(module, sv);
        }

        if (sv->setCb != NULL)
        {
            sv->setCb(&sv->value);
        }
    }

    _svarReleaseLock();
    return PASS;
}

int SVAR_GetIdByName(char *name)
{
    if (name == NULL)
    {
        return FAIL;
    }

    for (uint16_t i = 0; i < SVAR_MAX_MODULES; i++)
    {
        if (svarModulesRegistry[i] != NULL)
        {
            for (uint16_t j = 0; j < svarModulesRegistry[i]->count; j++)
            {
                system_variable_t *sv = &svarModulesRegistry[i]->table[j];
                if (sv->name != NULL && strcmp(sv->name, name) == 0)
                {
                    return (int)sv->id;
                }
            }
        }
    }

    return FAIL; // not found
}

 char* SVAR_GetName(uint32_t id)
{
    system_variable_t *sv = _getSvarFromId(id);

    if (sv == NULL)
    {
        return NULL;
    }

    return sv->name;
}

gsg_result_t SVAR_SetMin(uint32_t id, void *data)
{
    if (data == NULL)
    {
        return GSG_INVALID_ARG;
    }

    system_variable_t *sv = _getSvarFromId(id);
    if (sv == NULL)
    {
        return GSG_NOT_FOUND;
    }

    if (sv->flags.minWritable == 0)
    {
        return GSG_ERROR;
    }

    svar_value_t temp;
    _copyToUnion(sv, &temp, data);
    return _setMin(sv, &temp);
}

gsg_result_t SVAR_SetMax(uint32_t id, void *data)
{
    if (data == NULL)
    {
        return GSG_INVALID_ARG;
    }

    system_variable_t *sv = _getSvarFromId(id);
    if (sv == NULL)
    {
        return GSG_NOT_FOUND;
    }

    if (sv->flags.maxWritable == 0)
    {
        return GSG_ERROR;
    }

    svar_value_t temp;
    _copyToUnion(sv, &temp, data);
    return _setMax(sv, &temp);
}

gsg_result_t SVAR_SetDefault(uint32_t id, void *data)
{
    if (data == NULL)
    {
        return GSG_INVALID_ARG;
    }

    system_variable_t *sv = _getSvarFromId(id);
    if (sv == NULL)
    {
        return GSG_NOT_FOUND;
    }

    if (sv->flags.defWritable == 0)
    {
        return GSG_ERROR;
    }

    svar_value_t temp;
    _copyToUnion(sv, &temp, data);
    return _setDefault(sv, &temp);
}

gsg_result_t SVAR_GetMin(uint32_t id, void *data)
{
    if (data == NULL)
    {
        return GSG_INVALID_ARG;
    }

    system_variable_t *sv = _getSvarFromId(id);
    if (sv == NULL)
    {
        return GSG_NOT_FOUND;
    }

    svar_value_t temp;
    gsg_result_t result = _getMin(sv, &temp);
    if (result != GSG_SUCCESS)
    {
        return result;
    }

    _copyFromUnion(sv, data, &temp);
    return GSG_SUCCESS;
}

gsg_result_t SVAR_GetMax(uint32_t id, void *data)
{
    if (data == NULL)
    {
        return GSG_INVALID_ARG;
    }

    system_variable_t *sv = _getSvarFromId(id);
    if (sv == NULL)
    {
        return GSG_NOT_FOUND;
    }

    svar_value_t temp;
    gsg_result_t result = _getMax(sv, &temp);
    if (result != GSG_SUCCESS)
    {
        return result;
    }

    _copyFromUnion(sv, data, &temp);
    return GSG_SUCCESS;
}

gsg_result_t SVAR_GetDefault(uint32_t id, void *data)
{
    if (data == NULL)
    {
        return GSG_INVALID_ARG;
    }

    system_variable_t *sv = _getSvarFromId(id);
    if (sv == NULL)
    {
        return GSG_NOT_FOUND;
    }

    svar_value_t temp;
    gsg_result_t result = _getDefault(sv, &temp);
    if (result != GSG_SUCCESS)
    {
        return result;
    }

    _copyFromUnion(sv, data, &temp);
    return GSG_SUCCESS;
}

gsg_result_t SVAR_registerSetCallback(uint32_t id, svar_set_callback_t cb)
{
    system_variable_t *sv = _getSvarFromId(id);
    if (sv == NULL)
    {
        return GSG_NOT_FOUND;
    }

    sv->setCb = cb;
    return GSG_SUCCESS;
}

gsg_result_t SVAR_registerGetCallback(uint32_t id, svar_get_callback_t cb)
{
    system_variable_t *sv = _getSvarFromId(id);
    if (sv == NULL)
    {
        return GSG_NOT_FOUND;
    }

    sv->getCb = cb;
    return GSG_SUCCESS;
}

gsg_result_t SVAR_registerNvmDevice(svar_module_t *module, nvm_device_t *nvm)
{
    if (module == NULL) 
        return GSG_INVALID_ARG;

    if (nvm == NULL) 
        return GSG_INVALID_ARG;
    
    module->nvmHandle = nvm;
    
    return GSG_SUCCESS;
}

/***************************************************************************************************************
 * Sample command: <command> <key>,<id>,<value>  
 * Command => "setvar" "getvar"
 * key => "val" "def" "min" "max" "name"
 * id => svarId
 * value => value to be set (only for setvar command)
 * 
 * setvar response
 *      setvar key,id,<status>
 *      status => "OK" "ERR" "OUT_OF_RANGE"
 * 
 * getvar key,id,<value>
 *      value => value of the key requested (signed / unsigned / string / float based on the variable type)
 **************************************************************************************************************/
void svarGetCmdHandler(char *args)
{
    if (args == NULL)
    {
        return;
    }

    char *key = strtok(args, ",");
    char *idStr = strtok(NULL, ",");

    if (key == NULL || idStr == NULL)
    {
        DEBUG_LOGE(DEBUG_TAG_SVAR, "SVAR","Insufficient arguments");
        return;
    }

    uint32_t id = (uint32_t)atoi(idStr);

    system_variable_t *sv = _getSvarFromId(id);
    if (sv == NULL)
    {
        DEBUG_LOGE(DEBUG_TAG_SVAR, "SVAR","getvar %lu NOT FOUND", id);
        return;
    }

    char response[64] = {0};

    /* ---------- NAME ---------- */
    if (strcmp(key, "name") == 0)
    {
        snprintf(response, sizeof(response),
                 "getvar name,%lu,%s",
                 id, (sv->name != NULL) ? sv->name : "NULL");

        DEBUG_LOGI(DEBUG_TAG_SVAR, "SVAR","%s", response);
        return;
    }

    /* ---------- VALUE FETCH ---------- */
    svar_value_t temp;
    gsg_result_t res = GSG_ERROR;

    if (strcmp(key, "val") == 0)
        res = _getValue(sv, &temp);
    else if (strcmp(key, "def") == 0)
        res = _getDefault(sv, &temp);
    else if (strcmp(key, "min") == 0)
        res = _getMin(sv, &temp);
    else if (strcmp(key, "max") == 0)
        res = _getMax(sv, &temp);
    else
    {
        DEBUG_LOGE(DEBUG_TAG_SVAR, "SVAR","Unknown key");
        return;
    }

    if (res != GSG_SUCCESS)
    {
        DEBUG_LOGE(DEBUG_TAG_SVAR, "SVAR","getvar %s,%lu,ERR", key, id);
        return;
    }

    /* ---------- FORMAT ---------- */
    switch (sv->type)
    {
        case SVAR_TYPE_INT8:
            snprintf(response, sizeof(response), "getvar %s,%lu,%d", key, id, temp.i8);
            break;

        case SVAR_TYPE_UINT8:
            snprintf(response, sizeof(response), "getvar %s,%lu,%u", key, id, temp.u8);
            break;

        case SVAR_TYPE_INT16:
            snprintf(response, sizeof(response), "getvar %s,%lu,%d", key, id, temp.i16);
            break;

        case SVAR_TYPE_UINT16:
            snprintf(response, sizeof(response), "getvar %s,%lu,%u", key, id, temp.u16);
            break;

        case SVAR_TYPE_INT32:
            snprintf(response, sizeof(response), "getvar %s,%lu,%ld", key, id, temp.i32);
            break;

        case SVAR_TYPE_UINT32:
            snprintf(response, sizeof(response), "getvar %s,%lu,%lu", key, id, temp.u32);
            break;

#if SVAR_ENABLE_64_BIT
        case SVAR_TYPE_INT64:
            snprintf(response, sizeof(response), "getvar %s,%lu,%lld", key, id, temp.i64);
            break;

        case SVAR_TYPE_UINT64:
            snprintf(response, sizeof(response), "getvar %s,%lu,%llu", key, id, temp.u64);
            break;
#endif

        case SVAR_TYPE_FLOAT:
//            snprintf(response, sizeof(response), "getvar %s,%lu,%.3f", key, id, temp.f);
            break;

        case SVAR_TYPE_BOOL:
            snprintf(response, sizeof(response), "getvar %s,%lu,%u", key, id, temp.b);
            break;

        case SVAR_TYPE_CHAR:
            snprintf(response, sizeof(response), "getvar %s,%lu,%c", key, id, temp.c);
            break;

        case SVAR_TYPE_STRING:
            snprintf(response, sizeof(response), "getvar %s,%lu,%s",
                     key, id, (temp.str != NULL) ? temp.str : "NULL");
            break;

        default:
            DEBUG_LOGE(DEBUG_TAG_SVAR, "SVAR","Unsupported type");
            return;
    }

    DEBUG_LOGI(DEBUG_TAG_SVAR, "SVAR","%s", response);
}

#endif
