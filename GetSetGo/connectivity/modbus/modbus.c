#include "modbus.h"

// Private variables and function prototypes 
debugTagId_t 	debugTagId = DEBUG_TAG_COMM;
char 			debugTag[] = "MB";

static uint8_t portCount = 0; // Counter for the number of ports

#if (MODBUS_MASTER_MODE == ENABLED)
    static modbus_slave_info_t slaveInfo[MODBUS_MASTER_MAX_SLAVES]; // Array to hold slave information
    static uint8_t slaveCount = 0; // Counter for the number of slaves
#endif

// Private function prototypes
static void modbusTaskHandler(void *pvParameters);
void mbMasterPushQueryTimerCallback(void *arg);
void mbMasterQueryTimerHandler(void *arg);
void mbTxGenFrame(modbus_port_t *port, mb_query_type_t queryType, uint8_t slaveId, uint16_t address, uint16_t regCount);
gsg_result_t mbPhySendData(modbus_port_t *port, uint8_t *data, uint16_t size);


static void modbusTaskHandler(void * argument)
{
    // typecast the port parameter to modbus_port_t pointer
    modbus_port_t *modbusPort = (modbus_port_t *)argument;
    char tempBuffer[64]; // Temporary buffer for debug messages
    modbusPort->state = MB_PORT_STATE_MASTER_IDLE; // Set initial state to disabled

    // Modbus task implementation
    while (1)
    {
        static uint8_t prvState = 0xFF;
        if(modbusPort->state != prvState)
        {
            prvState = modbusPort->state; // Update previous state
            sprintf(tempBuffer,"State: %d", modbusPort->state);
            DEBUG_LOGI(DEBUG_TAG_COMM,"MB",tempBuffer);
        }

        // Modbus port state machine
        switch (modbusPort->state)
        {
            // Master Mode States

            case MB_PORT_STATE_MASTER_IDLE:
            {
                // Check if there is a query registered for master mode
                if (modbusPort->currentQuery.slaveId)
                {
                    modbusPort->state = MB_PORT_STATE_MASTER_TX_PROCESSING; // Set to processing state
                }
                break;
            }
            case MB_PORT_STATE_MASTER_TX_PROCESSING:
            {
                mbTxGenFrame(modbusPort, 
                                modbusPort->currentQuery.type, 
                                modbusPort->currentQuery.slaveId, 
                                modbusPort->currentQuery.address, 
                                modbusPort->currentQuery.regCount);
                
                modbusPort->state = MB_PORT_STATE_MASTER_TRANSMITTING;
                break;
            }
            case MB_PORT_STATE_MASTER_TRANSMITTING:
            {
                mbPhySendData(modbusPort, 
                                modbusPort->tx_buffer, 
                                modbusPort->tx_buffer_length);
                modbusPort->state = MB_PORT_STATE_MASTER_RX_WAITING;
                break;
            }
            case MB_PORT_STATE_MASTER_RX_WAITING:
            {
                modbusPort->state = MB_PORT_STATE_MASTER_RX_PROCESSING; // Set to processing state
                break;
            }
            case MB_PORT_STATE_MASTER_RX_PROCESSING:
            {
                modbusPort->state = MB_PORT_STATE_MASTER_RESET;
                break;
            }
            case MB_PORT_STATE_MASTER_RESET:
            {
                // Reset the Modbus port state
                // Clear the current query after processing
                memset(&modbusPort->currentQuery, 0, sizeof(mb_master_query_t));
                modbusPort->state = MB_PORT_STATE_MASTER_IDLE; // Reset to idle state
                break;
            }
            case MB_PORT_STATE_DISABLED:
            {
                // Port is disabled, do nothing
                break;
            }
            default:
                break;
        }
        osDelay(100);
    }
    osThreadExit(); // Exit the task when done
}

gsg_result_t MB_createPortStatic(modbus_port_t * port)
{
    // Validate the input struct
    if (portCount >= MODBUS_PORT_MAX_COUNT)
       return GSG_ERROR;

    if (port == NULL)
        return GSG_INVALID_ARG;

    if((port->mode == MODBUS_MODE_NONE) || (port->mode >= _MODBUS_MODE_MAX))
        return GSG_INVALID_ARG; // Invalid port type

    if((port->phy == MODBUS_PHY_NONE) || (port->phy >= _MODBUS_PHY_MAX))
        return GSG_INVALID_ARG; // Invalid port type

    port->state = MB_PORT_STATE_DISABLED; // Set initial state to disabled

    // Reset memory for the port buffers
//    memset(port->rx_buffer, 0, MODBUS_PORT_RX_BUFFER_SIZE);
//    memset(port->tx_buffer, 0, MODBUS_PORT_TX_BUFFER_SIZE);

    #if (MODBUS_MASTER_USE_UNIFIED_REGISTER_MAP == ENABLED)
        // Initialize registers, buffers, etc. as needed
        if(port->holdingRegistersCount  == 0)
            port->holdingRegisters       = NULL;

        if(port->inputRegistersCount   == 0)
            port->inputRegisters        = NULL;

        if(port->discreteInputsCount  == 0)
            port->discreteInputs       = NULL;

        if(port->coilsCount            == 0)
            port->coils                = NULL;
    #endif

    return GSG_SUCCESS;

}

gsg_result_t MB_startPort(modbus_port_t * port)
{
    // Validate the input struct
    if (port == NULL)
        return GSG_INVALID_ARG;
    if (port->state != MB_PORT_STATE_DISABLED)
        return GSG_ERROR; // Port is already started or in use

    // Start Modbus task
    const osThreadAttr_t defaultTask_attributes = {
      .name = "defaultTask",
      .stack_size = 2048,
      .priority = (osPriority_t) osPriorityNormal,
    };

    port->taskHandle = osThreadNew(modbusTaskHandler,
    		port,
			&defaultTask_attributes);
    if (port->taskHandle == NULL)
        return GSG_ERROR;

    // Start periodic query timer and save the handle
//    port->queryTimerHandle = osTimerNew(mbMasterQueryTimerHandler, osTimerPeriodic, port, NULL);
    if (port->queryTimerHandle == NULL)
        return GSG_ERROR;
//    osTimerStart(port->queryTimerHandle, 100); // 100 ms periodic timer
    return GSG_SUCCESS;
}

gsg_result_t MB_destroyPort(modbus_port_t * port)
{
    // Validate the input struct
    if (port == NULL)
        return GSG_INVALID_ARG;

    // Stop the Modbus task associated with this port
    if (port->taskHandle != NULL)
    {
        osThreadTerminate(port->taskHandle);
        port->taskHandle = NULL;
    }

    // Reset the port state
    port->state = MB_PORT_STATE_DISABLED;

    // Free any allocated resources if necessary
    #if MODBUS_PORT_USE_HEAP
        free(port->rx_buffer);
        free(port->tx_buffer);
    #endif

    return GSG_SUCCESS;
}

gsg_result_t MB_registerSlaveInfo(modbus_slave_info_t * slave )
{
    if (slaveCount >= MODBUS_MASTER_MAX_SLAVES)
        return GSG_OVERFLOW; // Maximum number of slaves reached

    // Check if the slave ID already exists
    for (uint8_t i = 0; i < slaveCount; i++)
    {
        if (slaveInfo[i].id == slave->id)
            return GSG_INVALID_ARG; // Slave ID already exists
    }

    // Register the new slave information
    slaveInfo[slaveCount].id                = slave->id;
    slaveInfo[slaveCount].phy               = slave->phy;
    slaveInfo[slaveCount].status            = 0; // Initialize status to 0
    
    #if (MODBUS_MASTER_USE_UNIFIED_REGISTER_MAP == DISABLED)

        if(slave->holdingRegistersCount > 0)
        {
            slaveInfo[slaveCount].holdingRegisters = slave->holdingRegisters; // Pointer to holding registers
            slaveInfo[slaveCount].holdingRegistersCount = slave->holdingRegistersCount; // Count of holding registers
        }
        else
        {
            slaveInfo[slaveCount].holdingRegisters = NULL; // Initialize to NULL
            slaveInfo[slaveCount].holdingRegistersCount = 0; // Initialize count to 0
        }
        if(slave->inputRegistersCount > 0)
        {
            slaveInfo[slaveCount].inputRegisters = slave->inputRegisters; // Pointer to input registers
            slaveInfo[slaveCount].inputRegistersCount = slave->inputRegistersCount; // Count of input registers
        }
        else
        {
            slaveInfo[slaveCount].inputRegisters = NULL; // Initialize to NULL
            slaveInfo[slaveCount].inputRegistersCount = 0; // Initialize count to 0
        }
        if(slave->coilsCount > 0)
        {
            slaveInfo[slaveCount].coils = slave->coils; // Pointer to coils
            slaveInfo[slaveCount].coilsCount = slave->coilsCount; // Count of coils
        }
        else
        {
            slaveInfo[slaveCount].coils = NULL; // Initialize to NULL
            slaveInfo[slaveCount].coilsCount = 0; // Initialize count to 0
        }
        if(slave->discreteInputsCount > 0)
        {
            slaveInfo[slaveCount].discreteInputs = slave->discreteInputs; // Pointer to discrete inputs
            slaveInfo[slaveCount].discreteInputsCount = slave->discreteInputsCount; // Count of discrete inputs
        }
        else
        {
            slaveInfo[slaveCount].discreteInputs = NULL; // Initialize to NULL
            slaveInfo[slaveCount].discreteInputsCount = 0; // Initialize count to 0
        }
    #else
        slaveInfo[slaveCount].holdingRegOffset = holdingRegOffset;
        slaveInfo[slaveCount].inputRegOffset   = inputRegOffset;
        slaveInfo[slaveCount].coilsOffset      = coilsOffset;
        slaveInfo[slaveCount].discreteInpOffset = discreteInpOffset;
    #endif

    slaveCount++;
    return GSG_SUCCESS;
}

gsg_result_t MB_unregisterSlaveInfo(uint8_t slaveId)
{
    for (uint8_t i = 0; i < slaveCount; i++)
    {
        if (slaveInfo[i].id == slaveId)
        {
            // Put reset value in that slave info
            slaveInfo[i].id = 0;
            slaveInfo[i].phy = MODBUS_PHY_NONE;
            slaveInfo[i].status = 0; // Reset status

            #if (MODBUS_MASTER_USE_UNIFIED_REGISTER_MAP == DISABLED)
                slaveInfo[i].holdingRegisters = NULL;
                slaveInfo[i].holdingRegistersCount = 0;
                slaveInfo[i].inputRegisters = NULL;
                slaveInfo[i].inputRegistersCount = 0;
                slaveInfo[i].coils = NULL;
                slaveInfo[i].coilsCount = 0;
                slaveInfo[i].discreteInputs = NULL;
                slaveInfo[i].discreteInputsCount = 0;
            #else
                slaveInfo[i].holdingRegOffset = 0;
                slaveInfo[i].inputRegOffset = 0;
                slaveInfo[i].coilsOffset = 0;
                slaveInfo[i].discreteInpOffset = 0;
            #endif

            slaveCount--;
            return GSG_SUCCESS;
        }
    }
    return GSG_INVALID_ARG; // Slave ID not found
}
