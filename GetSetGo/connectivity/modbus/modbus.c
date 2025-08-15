#include "modbus.h"

// Private variables and function prototypes 
debugTagId_t 	debugTagId = DEBUG_TAG_COMM;
char 			debugTag[] = "MB";

static uint8_t portCount = 0; // Counter for the number of ports

// Private function prototypes
static void modbusTaskHandler(void *pvParameters);
void mbMasterPushQueryTimerCallback(void *arg);
void mbMasterQueryTimerHandler(void *arg);
void mbTxGenFrame(modbus_port_t *port, mb_query_type_t queryType, uint8_t slaveId, uint16_t address, uint16_t regCount);
gsg_result_t mbPhySendData(modbus_port_t *port, uint8_t *data, uint16_t size);
gsg_result_t mbPhyPreRx(modbus_port_t *port, uint8_t *data, uint16_t size);
modbus_slave_info_t *mbGetSlaveInfo(modbus_port_t *port, uint8_t slaveId);

modbus_slave_info_t *mbGetSlaveInfo(modbus_port_t *port, uint8_t slaveId)
{
    for (uint8_t i = 0; i < MODBUS_MASTER_MAX_SLAVES; i++)
    {
        if (port->slave[i]->id == slaveId)
        {
            return port->slave[i];
        }
    }
    return NULL;
}

static void modbusTaskHandler(void * argument)
{
    // typecast the port parameter to modbus_port_t pointer
    modbus_port_t *modbusPort = (modbus_port_t *)argument;
    char tempBuffer[64]; // Temporary buffer for debug messages
    mb_master_query_t   queryInProcess;
    modbus_slave_info_t *slaveInProcess = NULL;
    modbusPort->state = MB_PORT_STATE_MASTER_IDLE; // Set initial state to disabled

    // Dummy query for testing
    // mb_master_query_t query;
    // query.address = 0;
    // query.regCount = 5;
    // query.type = MB_QUERY_READ_HOLDING_REGISTERS;
    // query.periodicity = MB_MASTER_QUERY_PERIOD_1_S;
    // query.slaveId = 1;
    // memcpy(&modbusPort->currentQuery, &query, sizeof(mb_master_query_t));

    // Modbus task implementation
    while (1)
    {
        static uint8_t prvState = 0xFF;
        if(modbusPort->state != prvState)
        {
            prvState = modbusPort->state; // Update previous state
            // sprintf(tempBuffer,"State: %d", modbusPort->state);
            // DEBUG_LOGI(DEBUG_TAG_MODBUS,"MB",tempBuffer);
        }

        // Modbus port state machine
        switch (modbusPort->state)
        {
            // Master Mode States
            case MB_PORT_STATE_MASTER_IDLE:
            {
                mb_master_query_t *queryPtr = NULL; // Temporary pointer to receive from queue

                // Wait up to 500ms for a query to be available in the queue
                if (osMessageQueueGet(modbusPort->queryQueueHandle,
                                    &queryPtr,
                                    NULL,
                                    500) == osOK)  // Timeout in ms
                {
                    // Copy the struct data from the queued pointer to local variable
                    if(queryPtr != NULL)
                    {
                        queryInProcess = *queryPtr;
                        // Check if slave is registered
                        slaveInProcess = mbGetSlaveInfo(modbusPort, queryInProcess.slaveId);
                        if (slaveInProcess == NULL)  // Slave not found,
                        {
                            DEBUG_LOGE(DEBUG_TAG_MODBUS,"MB", "Unknown Slave in query");
                            modbusPort->state = MB_PORT_STATE_MASTER_RESET;
                            break;
                        }
                        else 
                        {
                            ;
                        }
                        // Move to next state
                        modbusPort->state = MB_PORT_STATE_MASTER_TX_PROCESSING;
                    }
                }
                else
                {
                    // No query in queue within 500ms, remain idle
                }   
                break;
            }
            case MB_PORT_STATE_MASTER_TX_PROCESSING:
            {
                mbTxGenFrame(modbusPort,
                                queryInProcess.type,
                                queryInProcess.slaveId,
                                queryInProcess.address,
                                queryInProcess.regCount);

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
               modbus_phy_t phy = slaveInProcess->phy;
               mbPhyRxCbContext_t *ctx = modbusPort->rxCtx[phy];
               if (ctx == NULL || ctx->rxQueueHandle == NULL)
               {
                   DEBUG_LOGE(DEBUG_TAG_MODBUS, "MB", "No valid Rx context");
                   modbusPort->state = MB_PORT_STATE_MASTER_RESET;
                   break;
               }

               // 3. Wait for frame length from ISR
               uint16_t frameLen = 0;
               if (osMessageQueueGet(ctx->rxQueueHandle, &frameLen, NULL, MODBUS_MASTER_RESPONSE_TIMEOUT_MS) == osOK)
               {
                    mbPhyPreRx(modbusPort, ctx->rxBuffer, frameLen);
                    mbRxFrameParse(modbusPort, ctx->rxBuffer, frameLen);
                    modbusPort->state = MB_PORT_STATE_MASTER_RX_PROCESSING;
               }
               else
               {
                   DEBUG_LOGW(DEBUG_TAG_MODBUS, "MB", "Response timeout from slave");
                   modbusPort->state = MB_PORT_STATE_MASTER_RESET;
               }
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
                modbusPort->state = MB_PORT_STATE_MASTER_IDLE; // Reset to idle state
                osDelay(100);
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

    // Create event flags
    port->eventHandle = osEventFlagsNew(NULL);
    if (port->eventHandle == NULL)
        return GSG_ERROR;

    #if (MODBUS_MASTER_MODE == ENABLED)
        if(port->mode == MODBUS_MODE_MASTER)
        {
            // Create query queue (FIFO for pending queries)
            port->queryQueueHandle = osMessageQueueNew(
                MODBUS_MASTER_QUERY_QUEUE_LENGTH,             // max queue length
                sizeof(mb_master_query_t *),           // item size
                NULL
            );
            if (port->queryQueueHandle == NULL)
                return GSG_ERROR;
        }
    #endif

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
    {
    	DEBUG_LOGE(DEBUG_TAG_MODBUS,"MB","Error Creating task");
        return GSG_ERROR;
    }

    // Start periodic query timer and save the handle
    port->queryTimerHandle = osTimerNew(mbMasterQueryTimerHandler, osTimerPeriodic, port, NULL);
    if (port->queryTimerHandle == NULL)
        return GSG_ERROR;
    osTimerStart(port->queryTimerHandle, 100); // 100 ms periodic timer
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

