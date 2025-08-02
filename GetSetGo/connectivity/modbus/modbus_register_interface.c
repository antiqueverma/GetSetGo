
#include "modbus.h"

extern debugTagId_t 	debugTagId;
extern char 			debugTag[];

static uint8_t mb_reg_table_write(modbus_register_t *reg, void *value)
{
    if (reg == NULL || value == NULL) 
    {
        return 0; // Error: Null pointer
    }

    // Check if the register is writable
    if (!(reg->flags & MB_REG_FLAG_WRITE))
    {
        return 0; // Error: Register is not writable
    }

    // Check if the value is within the allowed range
    if (*(int16_t *)value < reg->min_value || *(int16_t *)value > reg->max_value)
    {
        return 0; // Error: Value out of range
    }

    switch (reg->data_type) 
    {
        case MB_REG_TYP_INT8:
            *(int8_t *)reg->value = *(int8_t *)value;
            break;
        case MB_REG_TYP_INT16:
            *(int16_t *)reg->value = *(int16_t *)value;
            break;
        case MB_REG_TYP_INT32:
            *(int32_t *)reg->value = *(int32_t *)value;
            break;
        case MB_REG_TYP_STRING:
        // For strings, we assume the max length is defined by reg->max_value
            strncpy((char *)reg->value, (char *)value, reg->max_value);
            break;
        case MB_REG_TYP_PTR:
            reg->value = value; // Pointer assignment
            break;
        default:
            return 0; // Error: Unsupported data type
    }

    // If the register is persistent, save the value to EEPROM
    if (reg->flags & MB_REG_FLAG_PERSISTENT)
    {
        ;// eeprom_write(reg->eeprom_address, reg->value, sizeof(reg->value));
    }

    // If the register has a notification flag, trigger a notification
    if (reg->flags & MB_REG_FLAG_NOTIFY)
    {
        ;// mb_notify_register_change(reg);
    }

    return 1; // Success
}

static uint8_t mb_reg_table_read(modbus_register_t *reg, void *value)
{
    if (reg == NULL || value == NULL) 
    {
        return 0; // Error: Null pointer
    }

    // Check if the register is readable
    if (!(reg->flags & MB_REG_FLAG_READ))
    {
        return 0; // Error: Register is not readable
    }

    switch (reg->data_type) 
    {
        case MB_REG_TYP_INT8:
            *(int8_t *)value = *(int8_t *)reg->value;
            break;
        case MB_REG_TYP_INT16:
            *(int16_t *)value = *(int16_t *)reg->value;
            break;
        case MB_REG_TYP_INT32:
            *(int32_t *)value = *(int32_t *)reg->value;
            break;
        case MB_REG_TYP_STRING:
            strncpy((char *)value, (char *)reg->value, reg->max_value);
            break;
        case MB_REG_TYP_PTR:
            value = reg->value; // Pointer assignment
            break;
        default:
            return 0; // Error: Unsupported data type
    }

    // If the register has a notification flag, trigger a notification
    if (reg->flags & MB_REG_FLAG_NOTIFY)
    {
        ;// mb_notify_register_change(reg);
    }

    return 1; // Success
}



