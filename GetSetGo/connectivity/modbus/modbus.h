
#ifndef MODBUS_H_
#define MODBUS_H_

#include "gsg_base.h"


// Modbus Library Configuration
#define MODBUS_TASK_PRIORITY                            osPriorityNormal
#define MODBUS_TASK_STACK_SIZE                          KB_2_B(6)
#define MODBUS_PORT_MAX_COUNT                           5 // Maximum number of Modbus ports
#define MODBUS_PORT_USE_HEAP                            DISABLED
#define MODBUS_MASTER_MODE                              ENABLED
#define MODBUS_MASTER_MAX_SLAVES                        10 // Maximum 255 slaves supported
#define MODBUS_MASTER_USE_UNIFIED_REGISTER_MAP          DISABLED
#define MODBUS_MASTER_QUERY_MAX                         10 // Maximum number of periodic queries

#define MODBUS_SLAVE_MODE                               ENABLED

#if MODBUS_MASTER_USE_UNIFIED_REGISTER_MAP 
    #error "Unified register map is not yet supported in this version of the Modbus library."
#endif

// Typedefs - enums

// Modbus Physical Layer Types
typedef enum {
    MODBUS_PHY_NONE = 0,
    MODBUS_PHY_SERIAL,
    MODBUS_PHY_TCP,
    MODBUS_PHY_UDP,
    _MODBUS_PHY_MAX
} modbus_phy_t;

// Modbus Communication Modes
typedef enum {
    MODBUS_MODE_NONE = 0,
    MODBUS_MODE_MASTER,
    MODBUS_MODE_SLAVE,
    MODBUS_MODE_MULTI_SLAVE,
    _MODBUS_MODE_MAX
} modbus_mode_t;

// Modbus Port States
typedef enum {
    MB_PORT_STATE_DISABLED = 0,

    // Master Mode States
    MB_PORT_STATE_MASTER_IDLE,              // Wait for a query trigger
    MB_PORT_STATE_MASTER_TX_PROCESSING,     // generate Modbus frame
    MB_PORT_STATE_MASTER_TRANSMITTING,      // Transmit the frame
    MB_PORT_STATE_MASTER_RX_WAITING,        // Wait for a response
    MB_PORT_STATE_MASTER_RX_PROCESSING,     // Process the received frame
    MB_PORT_STATE_MASTER_RESET,             // Reset the port state

    // Slave Mode States
    MB_PORT_STATE_SLAVE_IDLE,               // Wait for a request
    MB_PORT_STATE_SLAVE_RX_PROCESSING,      // Process the received request
    MB_PORT_STATE_SLAVE_TX_PROCESSING,      // Process the received request
    MB_PORT_STATE_SLAVE_TRANSMITTING,       // Transmit the response
    MB_PORT_STATE_SLAVE_RESET,              // Reset the port state

    // Multi-Slave Mode States
    MB_PORT_STATE_MULTI_SLAVE_IDLE,
    MB_PORT_STATE_MULTI_SLAVE_RX_PROCESSING,      // Process the received request
    MB_PORT_STATE_MULTI_SLAVE_TX_PROCESSING,      // Process the received request
    MB_PORT_STATE_MULTI_SLAVE_TRANSMITTING,       // Transmit the response
    MB_PORT_STATE_MULTI_SLAVE_RESET,              // Reset the port state
} modbus_port_state_t;

// Modbus Error Types
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

// Modbus Function Codes
typedef enum{
    MB_FUNC_READ_COILS = 0x01,        // Read Coils
    MB_FUNC_READ_DISCRETE_INPUTS = 0x02, // Read Discrete
    MB_FUNC_READ_HOLDING_REGISTERS = 0x03, // Read Holding Registers
    MB_FUNC_READ_INPUT_REGISTERS = 0x04, // Read Input Registers
    MB_FUNC_WRITE_SINGLE_COIL = 0x05, // Write Single Coil
    MB_FUNC_WRITE_SINGLE_REGISTER = 0x06, // Write Single Register
    MB_FUNC_WRITE_MULTIPLE_COILS = 0x0F, // Write Multiple Coils
    MB_FUNC_WRITE_MULTIPLE_REGISTERS = 0x10, // Write Multiple Registers
} modbus_func_code_t;

// Modbus Register Flags
typedef enum {
    MB_REG_FLAG_SWAP_BYTES, // Swap bytes for 16-bit registers
    MB_REG_FLAG_READ, 
    MB_REG_FLAG_WRITE,
    MB_REG_FLAG_NOTIFY,
    MB_REG_FLAG_PERSISTENT, // Register value is stored in EEPROM
} modbus_reg_flags_t;

// Modbus Register Types
typedef enum {
    MB_REG_TYP_BOOL,  // Boolean value (1 bit)
    MB_REG_TYP_INT8,  //  8-bit integer
    MB_REG_TYP_UINT8, //  8-bit unsigned integer
    MB_REG_TYP_INT16, //  16-bit integer  
    MB_REG_TYP_UINT16, //  16-bit unsigned integer  
    MB_REG_TYP_INT32, //  32-bit integer
    MB_REG_TYP_UINT32, //  32-bit unsigned integer
    MB_REG_TYP_STRING,
    MB_REG_TYP_PTR
} modbus_reg_type_t;

// Modbus Query Types
typedef enum{
    MB_QUERY_WRITE_COILS,
    MB_QUERY_READ_COILS,
    MB_QUERY_READ_DISCRETE_INPUTS,
    MB_QUERY_READ_INPUT_REGISTERS,
    MB_QUERY_READ_HOLDING_REGISTERS,
    MB_QUERY_WRITE_HOLDING_REGISTERS,
} mb_query_type_t;

// Modbus Master Query Periodicity
typedef enum{
    MB_MASTER_QUERY_APERIODIC,
    MB_MASTER_QUERY_PERIOD_100_MS,
    MB_MASTER_QUERY_PERIOD_200_MS,
    MB_MASTER_QUERY_PERIOD_500_MS,
    MB_MASTER_QUERY_PERIOD_1_S,
    MB_MASTER_QUERY_PERIOD_5_S,
    MB_MASTER_QUERY_PERIOD_10_S,
    MB_MASTER_QUERY_PERIOD_30_S,
    MB_MASTER_QUERY_PERIOD_1_MIN,
    MB_MASTER_QUERY_PERIOD_5_MIN,
} mb_master_query_periodicity_t;

typedef struct {
    void               *value;          // Value of the register
    modbus_reg_type_t   data_type; // Data type of the register
    uint16_t            min_value;      // Minimum value of the register
    uint16_t            max_value;      // Maximum value of the register
    uint32_t            eeprom_address; // EEPROM address for persistent storage
    uint16_t            flags;
} modbus_register_t;

typedef struct{
    uint8_t asciiMode:1;
    uint8_t encrypt:1;
    uint8_t multiPhy:1;
    uint8_t reserved:5; // Reserved for future use
} modbus_port_flags_t;

// Do not change these values, they are critical for proper Modbus operation
#define MODBUS_PORT_RX_BUFFER_SIZE 520 // Size of the receive buffer for each Modbus port
#define MODBUS_PORT_TX_BUFFER_SIZE 520 // Size of the transmit buffer for each Modbus port

typedef struct {
    uint8_t                 slaveId; // Slave ID for the query
    mb_query_type_t  		type;
    uint16_t                address;
    uint16_t                regCount;
    mb_master_query_periodicity_t periodicity;
    uint32_t lastExecutionTimeMs;
} mb_master_query_t;

typedef struct{
    // Modbus Port Configuration
    modbus_mode_t       mode;       // Mode of Modbus communication (Master, Slave, etc.)
    modbus_phy_t        phy;
    modbus_port_state_t state; // Current state of the port
    modbus_port_flags_t flags; // Flags for additional port configuration

    // Buffers for receiving and transmitting data
    #if MODBUS_PORT_USE_HEAP
        uint8_t * rx_buffer; // Pointer to the receive buffer
        uint8_t * tx_buffer; // Pointer to the transmit buffer       
    #else
        uint8_t  rx_buffer[MODBUS_PORT_RX_BUFFER_SIZE]; // Static receive buffer
        uint8_t  tx_buffer[MODBUS_PORT_TX_BUFFER_SIZE]; // Static transmit buffer
        uint16_t rx_buffer_length; // Length of the receive buffer
        uint16_t tx_buffer_length; // Length of the transmit buffer
    #endif

    osThreadId_t                taskHandle;
    const osThreadAttr_t        taskAttributes;

    #if (MODBUS_MASTER_MODE == ENABLED)
        osTimerId_t                 queryTimerHandle;
        mb_master_query_t           *queryList[MODBUS_MASTER_QUERY_MAX];
        mb_master_query_t           currentQuery; // Pointer to the currently executing query
    #endif

    #if MODBUS_MASTER_USE_UNIFIED_REGISTER_MAP
        // Modbus Register Tables
        modbus_register_t * holdingRegisters; // Pointer to the holding register table
        uint16_t holdingRegistersCount; // Number of holding registers

        modbus_register_t * inputRegisters;  // Pointer to the input register table
        uint16_t inputRegistersCount; // Number of input registers

        modbus_register_t * coils;           // Pointer to the coils table
        uint16_t coilsCount; // Number of coils

        modbus_register_t * discreteInputs;  // Pointer to the discrete inputs table
        uint16_t discreteInputsCount; // Number of discrete inputs
    #endif
    
} modbus_port_t;

typedef struct {

    uint8_t         id; // Slave ID
    uint8_t         status; // Slave status
    modbus_phy_t    phy;

    #if (MODBUS_MASTER_USE_UNIFIED_REGISTER_MAP == 0)
        modbus_register_t * holdingRegisters; // Pointer to the holding register table
        uint16_t holdingRegistersCount; // Number of holding registers

        modbus_register_t * inputRegisters;  // Pointer to the input register table
        uint16_t inputRegistersCount; // Number of input registers

        modbus_register_t * coils;           // Pointer to the coils table
        uint16_t coilsCount; // Number of coils

        modbus_register_t * discreteInputs;  // Pointer to the discrete inputs table
        uint16_t discreteInputsCount; // Number of discrete inputs
    #else
        uint16_t holdingRegOffset; // Offset for holding registers
        uint16_t inputRegOffset;   // Offset for input registers
        uint16_t coilsOffset;      // Offset for coils
        uint16_t discreteInpOffset; // Offset for discrete inputs
    #endif
} modbus_slave_info_t;




typedef void (*modbusTxCallback_t)(const uint8_t *data, uint16_t size);

// Modbus Library Function Prototypes
gsg_result_t MB_createPortStatic(modbus_port_t * port);
gsg_result_t MB_startPort(modbus_port_t * port);
gsg_result_t MB_destroyPort(modbus_port_t * port);
gsg_result_t MB_registerPhyTxCallback(modbus_phy_t phy, modbusTxCallback_t cb);
gsg_result_t MB_unregisterPhyTxCallback(modbus_phy_t phy);

 
#endif /* MODBUS_H_ */
