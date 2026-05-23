
#include "gsg_config.h"
#if (!defined(GSG_USE_SVAR) && (GSG_USE_SVAR == GSG_ENABLE))
#include <ctype.h>   // for isprint
#include <string.h>  // for strlen
#include <string.h>
#include "svar_internal.h"

extern system_variable_t svar_table[];

static system_variable_t* _getVar(uint32_t id)
{
    if (id == 0 || id >= SVAR_MAX_ID)
    {
        return NULL;
    }
    return &svar_table[id];
}



static int _validate(system_variable_t *sv, svar_value_t *val)
{
    if (sv == NULL || val == NULL)
    {
        return -1;
    }

    switch (sv->type)
    {
        case SVAR_TYPE_INT8:
            if (val->i8 < sv->min.i8 || val->i8 > sv->max.i8) return -1;
            break;

        case SVAR_TYPE_INT16:
            if (val->i16 < sv->min.i16 || val->i16 > sv->max.i16) return -1;
            break;

        case SVAR_TYPE_INT32:
            if (val->i32 < sv->min.i32 || val->i32 > sv->max.i32) return -1;
            break;
        #if SVAR_ENABLE_64_BIT
        case SVAR_TYPE_INT64:
            if (val->i64 < sv->min.i64 || val->i64 > sv->max.i64) return -1;
            break;
        case SVAR_TYPE_UINT64:
            if (val->u64 < sv->min.u64 || val->u64 > sv->max.u64) return -1;
            break;
        #endif

        case SVAR_TYPE_UINT8:
            if (val->u8 < sv->min.u8 || val->u8 > sv->max.u8) return -1;
            break;

        case SVAR_TYPE_UINT16:
            if (val->u16 < sv->min.u16 || val->u16 > sv->max.u16) return -1;
            break;

        case SVAR_TYPE_UINT32:
            if (val->u32 < sv->min.u32 || val->u32 > sv->max.u32) return -1;
            break;

        case SVAR_TYPE_FLOAT:
            if (val->f < sv->min.f || val->f > sv->max.f) return -1;
            break;

        case SVAR_TYPE_BOOL:
            if (!(val->b == 0 || val->b == 1)) return -1;
            break;

        case SVAR_TYPE_CHAR:
            if (!isprint((unsigned char)val->c)) return -1;
            break;

        case SVAR_TYPE_STRING:
        {
            if (val->str == NULL)
            {
                return -1;
            }

            uint16_t max_len = sv->max.u16;

            // length check
            if (strlen(val->str) >= max_len)
            {
                return -1;
            }

            // printable check
            for (uint16_t i = 0; val->str[i] != '\0'; i++)
            {
                if (!isprint((unsigned char)val->str[i]))
                {
                    return -1;
                }
            }
            break;
        }

        default:
            return -1;
    }

    return 0;
}

static void _genericSetCb(system_variable_t *sv)
{
    (void)sv;
    // future hooks
}

static void _nvmWrite(system_variable_t *sv)
{
    if (sv == NULL || sv->flags.persistent == 0)
    {
        return;
    }

    // Example:
    // EEPROM_Write(sv->nvmAddr, &sv->value, sizeof(svar_value_t));
}

static void _nvmRead(system_variable_t *sv)
{
    if (sv == NULL || sv->flags.persistent == 0)
    {
        return;
    }

    // Example:
    // EEPROM_Read(sv->nvmAddr, &sv->value, sizeof(svar_value_t));
}

void SVAR_Init(void)
{
    for (uint32_t i = 1; i < SVAR_MAX_ID; i++)
    {
        system_variable_t *sv = &svar_table[i];

        if (sv->name == NULL)
        {
            continue;
        }

        // copy default to value
        sv->value = sv->def;

        // load from NVM if persistent
        if (sv->flags.persistent == 1)
        {
            _nvmRead(sv);
        }
    }
}

static int _setValue(system_variable_t *sv, svar_value_t *val)
{
    if (_validate(sv, val) != 0)
    {
        return -1;
    }

    sv->value = *val;

    // user callback
    if (sv->setCb != NULL)
    {
        sv->setCb(val);
    }

    // generic callback
    _genericSetCb(sv);

    // NVM write
    if (sv->flags.persistent == 1)
    {
        _nvmWrite(sv);
    }

    return 0;
}

static int _getValue(system_variable_t *sv, svar_value_t *val)
{
    *val = sv->value;

    if (sv->getCb != NULL)
    {
        sv->getCb(val);
    }

    return 0;
}

const char* SVAR_GetName(uint32_t id)
{
    system_variable_t *sv = _getVar(id);

    if (sv == NULL)
    {
        return NULL;
    }

    return sv->name;
}

static void _copyToUnion(system_variable_t *sv, svar_value_t *dst, void *src)
{
    switch (sv->type)
    {
        case SVAR_TYPE_INT8:    dst->i8  = *(int8_t*)src; break;
        case SVAR_TYPE_INT16:   dst->i16 = *(int16_t*)src; break;
        case SVAR_TYPE_INT32:   dst->i32 = *(int32_t*)src; break;
        #if SVAR_ENABLE_64_BIT
        case SVAR_TYPE_INT64:   dst->i64 = *(int64_t*)src; break;
        case SVAR_TYPE_UINT64:  dst->u64 = *(uint64_t*)src; break;
        #endif
        case SVAR_TYPE_UINT8:   dst->u8  = *(uint8_t*)src; break;
        case SVAR_TYPE_UINT16:  dst->u16 = *(uint16_t*)src; break;
        case SVAR_TYPE_UINT32:  dst->u32 = *(uint32_t*)src; break;
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
        case SVAR_TYPE_INT16:   *(int16_t*)dst = src->i16; break;
        case SVAR_TYPE_INT32:   *(int32_t*)dst = src->i32; break;
        #if SVAR_ENABLE_64_BIT
        case SVAR_TYPE_INT64:   *(int64_t*)dst = src->i64; break;
        case SVAR_TYPE_UINT64:  *(uint64_t*)dst = src->u64; break;
        #endif
        case SVAR_TYPE_UINT8:   *(uint8_t*)dst  = src->u8; break;
        case SVAR_TYPE_UINT16:  *(uint16_t*)dst = src->u16; break;
        case SVAR_TYPE_UINT32:  *(uint32_t*)dst = src->u32; break;
        case SVAR_TYPE_FLOAT:   *(float*)dst = src->f; break;
        case SVAR_TYPE_BOOL:    *(uint8_t*)dst = src->b; break;
        case SVAR_TYPE_CHAR:    *(char*)dst = src->c; break;

        case SVAR_TYPE_STRING:
            strcpy((char*)dst, src->str);
            break;

        default:
            break;
    }
}

int SVAR_Set(uint32_t id, void *data)
{
    system_variable_t *sv = _getVar(id);

    if (sv == NULL || data == NULL)
    {
        return -1;
    }

    svar_value_t temp;

    _copyToUnion(sv, &temp, data);

    return _setValue(sv, &temp);
}

int SVAR_Get(uint32_t id, void *data)
{
    system_variable_t *sv = _getVar(id);

    if (sv == NULL || data == NULL)
    {
        return -1;
    }

    svar_value_t temp;

    if (_getValue(sv, &temp) != 0)
    {
        return -1;
    }

    _copyFromUnion(sv, data, &temp);

    return 0;
}

int SVAR_SetMin(uint32_t id, void *data)
{
    system_variable_t *sv = _getVar(id);

    if (sv == NULL || data == NULL)
    {
        return -1;
    }

    if (sv->flags.minWritable == 0)
    {
        return -1;
    }

    svar_value_t temp;
    _copyToUnion(sv, &temp, data);

    sv->min = temp;
    return 0;
}

int SVAR_SetMax(uint32_t id, void *data)
{
    system_variable_t *sv = _getVar(id);

    if (sv == NULL || data == NULL)
    {
        return -1;
    }

    if (sv->flags.maxWritable == 0)
    {
        return -1;
    }

    svar_value_t temp;
    _copyToUnion(sv, &temp, data);

    sv->max = temp;
    return 0;
}

int SVAR_SetDefault(uint32_t id, void *data)
{
    system_variable_t *sv = _getVar(id);

    if (sv == NULL || data == NULL)
    {
        return -1;
    }

    if (sv->flags.defWritable == 0)
    {
        return -1;
    }

    svar_value_t temp;
    _copyToUnion(sv, &temp, data);

    sv->def = temp;
    return 0;
}

int SVAR_GetMin(uint32_t id, void *data)
{
    system_variable_t *sv = _getVar(id);

    if (sv == NULL || data == NULL)
    {
        return -1;
    }

    _copyFromUnion(sv, data, &sv->min);
    return 0;
}


int SVAR_GetMax(uint32_t id, void *data)
{
    system_variable_t *sv = _getVar(id);

    if (sv == NULL || data == NULL)
    {
        return -1;
    }

    _copyFromUnion(sv, data, &sv->max);
    return 0;
}


int SVAR_GetDefault(uint32_t id, void *data)
{
    system_variable_t *sv = _getVar(id);

    if (sv == NULL || data == NULL)
    {
        return -1;
    }

    _copyFromUnion(sv, data, &sv->def);
    return 0;
}


static volatile uint8_t _svar_lock = 0;

void SVAR_Lock(void)
{
    _svar_lock = 1;
}

void SVAR_Unlock(void)
{
    _svar_lock = 0;
}


int SVAR_Reset(uint32_t id)
{
    system_variable_t *sv = _getVar(id);

    if (sv == NULL)
    {
        return -1;
    }

    // lock check (optional usage)
    if (_svar_lock)
    {
        return -1;
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

    // user callback
    if (sv->setCb != NULL)
    {
        sv->setCb(&sv->value);
    }

    // generic callback
    _genericSetCb(sv);

    // save to NVM if needed
    if (sv->flags.persistent == 1)
    {
        _nvmWrite(sv);
    }

    return 0;
}

int SVAR_ResetAll(void)
{
    if (_svar_lock)
    {
        return -1;
    }

    for (uint32_t i = 1; i < SVAR_MAX_ID; i++)
    {
        system_variable_t *sv = &svar_table[i];

        if (sv->name == NULL)
        {
            continue;
        }

        if (sv->type == SVAR_TYPE_STRING)
        {
            strncpy(sv->value.str, sv->def.str, sv->max.u16);
            sv->value.str[sv->max.u16 - 1] = '\0';
        }
        else
        {
            sv->value = sv->def;
        }

        // optional: call cb per variable
        if (sv->setCb != NULL)
        {
            sv->setCb(&sv->value);
        }

        _genericSetCb(sv);

        if (sv->flags.persistent == 1)
        {
            _nvmWrite(sv);
        }
    }

    return 0;
}

int SVAR_GetIdByName(const char *name)
{
    if (name == NULL)
    {
        return -1;
    }

    for (uint32_t i = 1; i < SVAR_MAX_ID; i++)
    {
        system_variable_t *sv = &svar_table[i];

        if (sv->name == NULL)
        {
            continue;
        }

        if (strcmp(sv->name, name) == 0)
        {
            return (int)i;
        }
    }

    return -1; // not found
}

#endif
