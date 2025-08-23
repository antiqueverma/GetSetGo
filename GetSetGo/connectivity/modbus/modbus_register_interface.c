
#include "modbus.h"

void mb_notifyRegisterAccess(uint16_t regAddress, mb_query_type_t queryType);
modbus_error_t mb_regWrite(modbus_slave_info_t *slave, mb_query_type_t queryType, uint16_t regAddress, uint8_t regCount, uint8_t *value);
modbus_error_t mb_regRead(modbus_slave_info_t *slave, mb_query_type_t queryType, uint16_t regAddress, uint8_t regCount, uint8_t *value);



static inline void modbus_write_u16(uint8_t *dest, uint16_t value)
{
    dest[0] = (uint8_t)(value >> 8);   // High byte first
    dest[1] = (uint8_t)(value & 0xFF); // Low byte next
}

static inline uint16_t modbus_read_u16(const uint8_t *src)
{
    return (uint16_t)((src[0] << 8) | src[1]);
}

static inline void modbus_write_u32(uint8_t *dest, uint32_t value)
{
    dest[0] = (uint8_t)(value >> 24);
    dest[1] = (uint8_t)(value >> 16);
    dest[2] = (uint8_t)(value >> 8);
    dest[3] = (uint8_t)(value & 0xFF);
}

static inline uint32_t modbus_read_u32(const uint8_t *src)
{
    return ((uint32_t)src[0] << 24) |
           ((uint32_t)src[1] << 16) |
           ((uint32_t)src[2] << 8)  |
           ((uint32_t)src[3]);
}


modbus_error_t mb_regWrite(modbus_slave_info_t *slave, mb_query_type_t queryType, uint16_t regAddress, uint8_t regCount, uint8_t *frame)
{
    if (slave == NULL || frame == NULL)
    {
        return GSG_ERROR_NULL_POINTER;
    }

    modbus_register_t *table = NULL;
    if(queryType == MB_QUERY_READ_HOLDING_REGISTERS || queryType == MB_QUERY_WRITE_HOLDING_REGISTERS)
        table = slave->holdingRegisters;
    else if(queryType == MB_QUERY_READ_INPUT_REGISTERS)
        table = slave->inputRegisters;
    else
        return MB_ERROR_INVALID_ADDRESS;

    modbus_error_t result = MB_ERROR_NONE;


//    char hex[50];
//    sprintf(hex, "RegW:Id=%d, T=%d, A=%d[%d]", slave->id, queryType, regAddress, regCount);DEBUG_LOGD(DEBUG_TAG_COMM,"MB", hex);

//    DEBUG_LOGD(DEBUG_TAG_COMM,"MB", "Rx>");
//    for(uint8_t i=0 ; i<regCount*2 ; i++ )
//    {
// 		sprintf(hex, "   [%d]",frame[i]);
// 		DEBUG_LOG_RAW(hex);
//    }


    uint8_t regToDo = 0;
    for (regToDo = 0; regToDo < regCount; )
    {
        uint16_t baseIndex = regAddress + regToDo;   // << add this line
        modbus_register_t *reg = &table[baseIndex];
        regToDo++;  // For next iteration
        
        // Check if the register is writable
        if(!reg->flags.write)
        {
            if( reg->data_type == MB_REG_TYP_UINT8  || reg->data_type == MB_REG_TYP_INT8 
             || reg->data_type == MB_REG_TYP_UINT16 || reg->data_type == MB_REG_TYP_INT16
             || reg->data_type == MB_REG_TYP_STRING)
            {
                frame += sizeof(uint16_t);
            }
            else if (reg->data_type == MB_REG_TYP_UINT32 || reg->data_type == MB_REG_TYP_INT32)
            {
                frame += sizeof(uint32_t);
                regToDo++;
            }
            continue;
        }

        switch (reg->data_type) 
        {
            case MB_REG_TYP_BOOL:   // set the flag
            {
                // For boolean registers, we expect a Upper byte of 16bit register to contain a number representing the bit status
                // and the lower byte of the 16bit register represents the bit location
                uint8_t bitLocation = frame[1];
                uint8_t bitStatus = frame[0] ? 1 : 0;

                // sprintf(hex, "bool=%d[%d]",bitLocation,bitStatus);   DEBUG_LOGD(DEBUG_TAG_COMM,"MB", hex);

                if (reg->max_value == 8) // 8-bit register
                {
                    if (bitLocation > 7)
                        bitLocation = 7;
                    if (bitStatus)
                        *(uint8_t *)reg->value |= (1 << bitLocation);
                    else
                        *(uint8_t *)reg->value &= ~(1 << bitLocation);
                }
                else  if (reg->max_value == 16) // 16-bit register
                {
                    if (bitLocation > 15)
                        bitLocation = 15;
                    if (bitStatus)
                        *(uint16_t *)reg->value |= (1 << bitLocation);
                    else
                        *(uint16_t *)reg->value &= ~(1 << bitLocation);
                }

                frame += sizeof(uint16_t);
                break;
            }
            case MB_REG_TYP_UINT8:
            {
                uint16_t raw = modbus_read_u16(frame);
                uint8_t newValue = (uint8_t)raw;
                if(reg->flags.validate)
                {
                    // Validate the new value against min and max
                    if (newValue < reg->min_value || newValue > reg->max_value)
                    {
                        result = MB_ERROR_INVALID_VALUE;
                        continue;  // Move to the next register
                    }
                }
                *(uint8_t *)reg->value = newValue;
                frame += sizeof(uint16_t); // Move buffer pointer by 2bytes for next register
                break;
            }
            case MB_REG_TYP_INT8:
            {
                uint16_t raw = modbus_read_u16(frame);
                int8_t newValue = (int8_t)raw;
                if(reg->flags.validate)
                {
                    // Validate the new value against min and max
                    if (newValue < reg->min_value || newValue > reg->max_value)
                    {
                        result = MB_ERROR_INVALID_VALUE;
                        continue;  // Move to the next register
                    }
                }
                *(int8_t *)reg->value = newValue;
                frame += sizeof(uint16_t); // Move buffer pointer by 2bytes for next register
                break;
            }
            case MB_REG_TYP_UINT16:
            {
                uint16_t newValue = modbus_read_u16(frame);
                if(reg->flags.validate)
                {
                    // Validate the new value against min and max
                    if (newValue < reg->min_value || newValue > reg->max_value)
                    {
                        result = MB_ERROR_INVALID_VALUE;
                        continue;  // Move to the next register
                    }
                }
                *(uint16_t *)reg->value = newValue;

                frame += sizeof(uint16_t); // Move buffer pointer by 2bytes for next register
                break;
            }
            case MB_REG_TYP_INT16:
            {
                int16_t newValue = (int16_t)modbus_read_u16(frame);
                if(reg->flags.validate)
                {
                    // Validate the new value against min and max
                    if (newValue < reg->min_value || newValue > reg->max_value)
                    {
                        result = MB_ERROR_INVALID_VALUE;
                        continue;  // Move to the next register
                    }
                }
                *(int16_t *)reg->value = newValue;
                frame += sizeof(int16_t); // Move buffer pointer by 2bytes for next register
                break;
            }
            case MB_REG_TYP_UINT32:
            {
                uint32_t newValue = modbus_read_u32(frame);
                if(reg->flags.validate)
                {
                    // Validate the new value against min and max
                    if (newValue < reg->min_value || newValue > reg->max_value)
                    {
                        result = MB_ERROR_INVALID_VALUE;
                        continue;  // Move to the next register
                    }
                }
                *(uint32_t *)reg->value = newValue;
                frame += sizeof(uint32_t); // Move buffer pointer by 4bytes for next register
                regToDo++;      // 32bit variables occupy 2 registers
                break;
            }
            case MB_REG_TYP_INT32:
            {
                int32_t newValue = (int32_t)modbus_read_u32(frame);
                if(reg->flags.validate) // Validate the new value against min and max
                {
                    if (newValue < reg->min_value || newValue > reg->max_value)
                    {
                        result = MB_ERROR_INVALID_VALUE;
                        continue;  // Move to the next register
                    }
                }
                *(int32_t *)reg->value = newValue;
                frame += sizeof(int32_t); // Move buffer pointer by 4bytes for next register
                regToDo++;      // 32bit variables occupy 2 registers
                break;
            }
            case MB_REG_TYP_STRING:
            {
                // Manually copy two bytes
                *(uint8_t *)reg->value       = frame[0];
                *((uint8_t *)reg->value + 1) = frame[1];
                frame += sizeof(uint16_t); // Move buffer pointer by 2bytes for next register
                break;
            }
            default:
            {
                DEBUG_LOGE(DEBUG_TAG_MODBUS,"MB","Unsupported data type");
                frame += sizeof(uint16_t); // Move buffer pointer by 2bytes for next register
                continue; // Unsupported data type
            }
        }

        // If the register is persistent, save the value to EEPROM
        if (reg->flags.persistent)
        {
            ;// eeprom_write(reg->eeprom_address, reg->value, sizeof(reg->value));
        }

        // If the register has a notification flag, trigger a notification
        if (reg->flags.notify)
        {
            mb_notifyRegisterAccess(baseIndex, queryType);
        }
    }

    return result; // Success
}

modbus_error_t mb_regRead(modbus_slave_info_t *slave, mb_query_type_t queryType, uint16_t regAddress, uint8_t regCount, uint8_t *frame)
{
    if (slave == NULL || frame == NULL) 
    {
        return GSG_ERROR_NULL_POINTER;
    }

    modbus_register_t *table = NULL;
    if(queryType == MB_QUERY_READ_HOLDING_REGISTERS || queryType == MB_QUERY_WRITE_HOLDING_REGISTERS)
        table = slave->holdingRegisters;
    else if(queryType == MB_QUERY_READ_INPUT_REGISTERS)
        table = slave->inputRegisters;
    else
        return MB_ERROR_INVALID_ADDRESS;

    if((table == NULL) || (regAddress + regCount > slave->holdingRegistersCount))
    {
        return MB_ERROR_INVALID_ADDRESS;
    }

    modbus_error_t result = MB_ERROR_NONE;
    uint8_t regToDo = 0;

    // char hex[50];
    // sprintf(hex, "Id=%d, Type=%d, Add=%d[%d]", slave->id, queryType, regAddress, regCount);
    // DEBUG_LOGD(DEBUG_TAG_COMM,"MB", hex);

    for (regToDo = 0; regToDo < regCount;)
    {
        uint16_t baseIndex = regAddress + regToDo;   // << add this line
        modbus_register_t *reg = &table[baseIndex];
        regToDo++;  // For next iteration
        
        // Check if the register is readable
        if (!reg->flags.read)
        {
            if( reg->data_type == MB_REG_TYP_UINT8  || reg->data_type == MB_REG_TYP_INT8 
             || reg->data_type == MB_REG_TYP_UINT16 || reg->data_type == MB_REG_TYP_INT16
             || reg->data_type == MB_REG_TYP_STRING)
            {
                frame += sizeof(uint16_t);
            }
            else if (reg->data_type == MB_REG_TYP_UINT32 || reg->data_type == MB_REG_TYP_INT32)
            {
                frame += sizeof(uint32_t);
            }
            continue;
        }

        switch (reg->data_type)
        {
            case MB_REG_TYP_BOOL:  // Giving all flags as a 16-bit value
            {
                if(reg->max_value == 8)
                {
                    uint8_t val = *(uint8_t *)reg->value;
                    modbus_write_u16(frame, (uint16_t)val);
                }
                else if (reg->max_value == 16)
                {
                    uint16_t val = *(uint16_t *)reg->value;
                    modbus_write_u16(frame, val);
                }
                frame += sizeof(uint16_t);
                break;
            }
            case MB_REG_TYP_UINT8:
            {
                // 8bit variables also occupy 1 register
                uint8_t val = *(uint8_t *)reg->value;
                modbus_write_u16(frame, val);
                frame += sizeof(uint16_t);
                break;
            }
            case MB_REG_TYP_INT8:
            {
                int8_t val = *(int8_t *)reg->value;
                modbus_write_u16(frame, (uint16_t)val);
                frame += sizeof(uint16_t);
                break;
            }
            case MB_REG_TYP_UINT16:
            {   
                uint16_t val = *(uint16_t *)reg->value;
                modbus_write_u16(frame, val);
                frame += sizeof(uint16_t);
                break;
            }
            case MB_REG_TYP_INT16:
            {
                int16_t val = *(int16_t *)reg->value;
                modbus_write_u16(frame, (uint16_t)val);
                frame += sizeof(uint16_t);
                break;
            }
            case MB_REG_TYP_UINT32:
            {
                uint32_t val = *(uint32_t *)reg->value;
                modbus_write_u32(frame, val);
                regToDo++;      // 32bit variables occupy 2 registers
                frame += sizeof(uint32_t);
                break;
            }
            case MB_REG_TYP_INT32:
            {
                int32_t val = *(int32_t *)reg->value;
                modbus_write_u32(frame, (uint32_t)val);
                regToDo++;      // 32bit variables occupy 2 registers
                frame += sizeof(uint32_t);
                break;
            }
            case MB_REG_TYP_STRING:
            {
                // Manually copy two bytes
                frame[0] = *((uint8_t *)reg->value);
                frame[1] = *((uint8_t *)reg->value + 1);
                frame += sizeof(uint16_t); // Move pointer for next register
                break;
            }
            default:
            {
                DEBUG_LOGE(DEBUG_TAG_MODBUS,"MB","Unsupported data type");
                frame += sizeof(uint16_t);
                continue; // Unsupported data type
            }
        }

        // If the register has a notification flag, trigger a notification
        if (reg->flags.notify)
        {
            mb_notifyRegisterAccess(baseIndex, queryType);
        }
    }

    return result; // Success
}

void mb_notifyRegisterAccess(uint16_t regAddress, mb_query_type_t queryType)
{
    ;
}



