#include "modbus.h"

modbus_register_t HOLDING_REGISTERS[] = {
//  Address         VarPtr              Datatype            MinValue    MaxValue    EEPROM Address  Flags
    [0]         = { NULL,               MB_REG_TYP_INT16,   0,          0,          0,              MB_REG_FLAG_READ|MB_REG_FLAG_WRITE},
};

uint16_t HOLDING_REGISTERS_COUNT = sizeof(HOLDING_REGISTERS) / sizeof(modbus_register_t);

modbus_register_t INPUT_REGISTERS[] = {
//  Address         VarPtr              Datatype            MinValue    MaxValue    EEPROM Address  Flags
    [0]         = { NULL,               MB_REG_TYP_INT16,   0,          0,          0,              MB_REG_FLAG_READ},
};

uint16_t INPUT_REGISTERS_COUNT = sizeof(INPUT_REGISTERS) / sizeof(modbus_register_t);

modbus_register_t COILS[] = {
//  Address         VarPtr              Datatype            MinValue    MaxValue    EEPROM Address  Flags
    [0]         = { NULL,               MB_REG_TYP_BOOL,    0,          1,          0,              MB_REG_FLAG_READ|MB_REG_FLAG_WRITE},
};

uint16_t COILS_COUNT = sizeof(COILS) / sizeof(modbus_register_t);
