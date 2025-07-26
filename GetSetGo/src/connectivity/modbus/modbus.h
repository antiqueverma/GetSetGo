
#ifndef MODBUS_H_
#define MODBUS_H_

#include "main.h"


// Modbus Library Configuration
#define MODBUS_TASK_PRIORITY    (tskIDLE_PRIORITY + 1)
#define MODBUS_TASK_STACK_SIZE  KB_2_B(4)
#define MODBUS_PORT_MAX_COUNT   10 // Maximum number of Modbus ports

// Typedefs
typedef enum {
    MODBUS_PHY_NONE = 0,
    MODBUS_PHY_SERIAL_RTU,
    MODBUS_PHY_SERIAL_ASCII,
    MODBUS_PHY_TCP_RTU,
    MODBUS_PHY_UDP,

    MODBUS_PORT_TCP,
    MODBUS_PORT_RTU,
    MODBUS_PORT_ASCII
} modbus_phy_t;

typedef enum {
    MODBUS_MODE_NONE = 0,
    MODBUS_MODE_MASTER,
    MODBUS_MODE_SLAVE,
    MODBUS_MODE_MULTI_MASTER,
    MODBUS_MODE_MULTI_SLAVE
} modbus_mode_t;

typedef enum {
    MB_PORT_STATE_DISABLED = 0,
    MB_PORT_STATE_RX_WAITING,
    MB_PORT_STATE_RX_PROCESSING,
    MB_PORT_STATE_TX_PROCESSING,
    MB_PORT_STATE_TRANSMITTING,
} modbus_port_state_t;

// Modbus Library typedefs
typedef struct{
    modbus_port_type_t  type;  // Type of Modbus port (UART, TCP, RTU, ASCII)
    modbus_mode_t       mode;       // Mode of Modbus communication (Master, Slave, etc.)
    modbus_phy_t        phy;
    modbus_port_state_t state; // Current state of the port

    uint8_t * rx_buffer; // Pointer to the receive buffer
    uint8_t * tx_buffer; // Pointer to the transmit buffer       

    modbus_register_t * holdingRegisters; // Pointer to the holding register table
    uint16_t holdingRegistersCount; // Number of holding registers
    modbus_register_t * inputRegisters;  // Pointer to the input register table
    uint16_t inputRegistersCount; // Number of input registers
    modbus_register_t * coils;           // Pointer to the coils table
    uint16_t coilsCount; // Number of coils
    modbus_register_t * discreteInputs;  // Pointer to the discrete inputs table
    uint16_t discreteInputsCount; // Number of discrete inputs
    
} modbus_port_t;

typedef enum{
    MB_ERROR_NONE = 0,
    MB_ERROR_INVALID_FRAME,
    MB_ERROR_TIMEOUT,
    MB_ERROR_CRC_MISMATCH,
    MB_ERROR_PORT_NOT_FOUND,
    MB_ERROR_BUFFER_OVERFLOW,
    MB_ERROR_INVALID_REGISTER,
    MB_ERROR_INVALID_ADDRESS,
    MB_ERROR_INVALID_VALUE,
    MB_ERROR_INVALID_FUNCTION_CODE,
} modbus_error_t;

typedef enum{
    MB_FUNC_READ_COILS = 0x01,        // Read Coils
    MB_FUNC_READ_DISCRETE_INPUTS = 0x02, // Read Discrete
    MB_FUNC_READ_HOLDING_REGISTERS = 0x03, // Read Holding Registers
    MB_FUNC_READ_INPUT_REGISTERS = 0x04, // Read Input Registers
    MB_FUNC_WRITE_SINGLE_COIL = 0x05, // Write Single Coil
    MB_FUNC_WRITE_SINGLE_REGISTER = 0x06, // Write Single Register
    MB_FUNC_WRITE_MULTIPLE_COILS = 0x0F, // Write Multiple Coils
    MB_FUNC_WRITE_MULTIPLE_REGISTERS = 0x10, // Write Multiple Registers

} modbus_function_code_t;

typedef enum {
    MB_REG_FLAG_SWAP_BYTES, // Swap bytes for 16-bit registers
    MB_REG_FLAG_READ, 
    MB_REG_FLAG_WRITE,
    MB_REG_FLAG_NOTIFY,
    MB_REG_FLAG_PERSISTENT, // Register value is stored in EEPROM
} modbus_register_flags_t;

typedef enum {
    MB_REG_TYP_BOOL,  // Boolean value (1 bit)
    MB_REG_TYP_INT8,  //  8-bit integer
    MB_REG_TYP_INT16, //  16-bit integer    
    MB_REG_TYP_INT32, //  32-bit integer
    MB_REG_TYP_STRING,
    MB_REG_TYP_PTR
} modbus_register_data_type_t;

typedef struct {
    void    *value;          // Value of the register
    modbus_register_data_type_t data_type; // Data type of the register
    uint16_t min_value;      // Minimum value of the register
    uint16_t max_value;      // Maximum value of the register
    uint32_t eeprom_address; // EEPROM address for persistent storage        
    uint16_t  flags; 
} modbus_register_t;

// Modbus Library Function Prototypes
void MB_Init(void);
void MB_createPort(void);


#endif /* MODBUS_H_ */