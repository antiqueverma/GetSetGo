
#include "modbus.h"


void mb_masterProcessConfigRequest(modbus_port_t *port, mb_config_request_t *configRqst);
modbus_slave_info_t *mbGetSlaveInfo(modbus_port_t *port, uint8_t slaveId);
gsg_result_t mbPhyPreRx(modbus_port_t *port, uint8_t *data, uint16_t size);
gsg_result_t mbPhySendData(modbus_port_t *port, uint8_t *data, uint16_t size);
void mbTxGenFrame(modbus_port_t *port, mb_query_type_t queryType, uint8_t slaveId, uint16_t address, uint16_t regCount);
modbus_error_t mbRxFrameParse(modbus_port_t *port, uint8_t slaveId, modbus_func_code_t funcCode, uint16_t address, uint16_t regCount);


void modbusMasterTaskHandler(void * argument)
{
    // typecast the port parameter to modbus_port_t pointer
    modbus_port_t *modbusPort = (modbus_port_t *)argument;
    char tempBuffer[100]; // Temporary buffer for debug messages
    mb_master_query_t   queryInProcess;
    modbus_slave_info_t *slaveInProcess = NULL;
    modbusPort->state = MB_PORT_STATE_MASTER_IDLE; // Set initial state to disabled
    mbPhyRxCbContext_t *ctx = NULL;
    mb_config_request_t configRqst;

    TickType_t slaveResponseTimer = 0;
    TickType_t interFrameDelayTimer = 0;
    

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
            //  sprintf(tempBuffer,"State: %d", modbusPort->state);
            //  DEBUG_LOGI(DEBUG_TAG_MODBUS,"MBM",tempBuffer);
            //  vTaskDelay(pdMS_TO_TICKS(100));
        }

        // Modbus port state machine
        switch (modbusPort->state)
        {
            // Master Mode States
            case MB_PORT_STATE_MASTER_IDLE:
            {
            	 // Check for any configuration requests and process them all at once
				while (xQueueReceive(modbusPort->configRqstQueHandle, &configRqst, 0) == pdTRUE)
				{
                    sprintf(tempBuffer, "New Rqst:%d", configRqst.rqstType);
                    DEBUG_LOGD(DEBUG_TAG_MODBUS,"MB",tempBuffer);

					// Process the configuration request
					mb_masterProcessConfigRequest(modbusPort, &configRqst);
				}

                // Check if we are waiting for a slave to respond
                if (slaveResponseTimer)
                {
                    vTaskDelay(pdMS_TO_TICKS(MODBUS_MASTER_IDLE_DELAY_MS));
                    modbusPort->state = MB_PORT_STATE_MASTER_RX_WAITING;
                    break;
                }
                else if ((!interFrameDelayTimer) || ((xTaskGetTickCount() - interFrameDelayTimer) >= MODBUS_MASTER_INTER_FRAME_DELAY_MS))
                {
                    interFrameDelayTimer = 0;
                    modbusPort->state = MB_PORT_STATE_MASTER_QUERY_WAITING;
                }
                vTaskDelay(pdMS_TO_TICKS(MODBUS_MASTER_IDLE_DELAY_MS));    
                break;
            }
            case MB_PORT_STATE_MASTER_QUERY_WAITING:
            {
                mb_master_query_t *queryPtr = NULL; // Temporary pointer to receive from queue
                // Wait up to 50ms for a query to be available in the queue
                if (xQueueReceive(modbusPort->queryQueueHandle,
                                    &queryPtr, 
                                    0) == pdTRUE)  // Timeout in ms
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
                        // Move to next state
                        modbusPort->state = MB_PORT_STATE_MASTER_TX_PROCESSING;
                    }
                }
                else
                {
                    modbusPort->state = MB_PORT_STATE_MASTER_IDLE;
                }
                break;
            }
            case MB_PORT_STATE_MASTER_TX_PROCESSING:
            {
            	modbus_phy_t phy = slaveInProcess->phy;
				ctx = modbusPort->rxCtx[phy];
				if (ctx == NULL || ctx->rxQueueHandle == NULL)
				{
					DEBUG_LOGE(DEBUG_TAG_MODBUS, "MB", "No valid Rx context");
					modbusPort->state = MB_PORT_STATE_MASTER_RESET;
					break;
				}
				

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
                slaveResponseTimer = xTaskGetTickCount();       // Start the timer 
                interFrameDelayTimer = xTaskGetTickCount();       // Start the timer    
                modbusPort->state = MB_PORT_STATE_MASTER_RX_WAITING;
                break;
            }
            case MB_PORT_STATE_MASTER_RX_WAITING:
            {
            	uint8_t byte = 0;
                TickType_t currentTime = xTaskGetTickCount();

                if (xQueueReceive(ctx->rxQueueHandle, &byte, 0) == pdTRUE)
                {
                	modbusPort->rx_buffer[modbusPort->rx_buffer_length++] = byte;
					while (xQueueReceive(ctx->rxQueueHandle, &byte, pdMS_TO_TICKS(MODBUS_MASTER_INTER_BYTE_TIMEOUT_MS)) == pdTRUE)
					{
						if (modbusPort->rx_buffer_length < sizeof(modbusPort->rx_buffer))
						    modbusPort->rx_buffer[modbusPort->rx_buffer_length++] = byte;
						else
						    DEBUG_LOGE(DEBUG_TAG_MODBUS, "MB", "RX buffer overflow");
					}
                    // Data received, move to processing state
                    modbusPort->state = MB_PORT_STATE_MASTER_RX_PROCESSING;
                    break;
                }
                else if((currentTime - slaveResponseTimer) > pdMS_TO_TICKS(MODBUS_MASTER_RESPONSE_TIMEOUT_MS)) // Slave hasn't responded at all
                {
                    // Timeout happened
                    slaveResponseTimer = 0;
                    if(modbusPort->rx_buffer_length == 0)
                    {
                        DEBUG_LOGW(DEBUG_TAG_MODBUS, "MB", "Response timeout from slave");
                        slaveInProcess->status.connected = 0; // Mark the slave as disconnected
                        modbusPort->state = MB_PORT_STATE_MASTER_RESET;
                        break;
                    }
                }
                else
                {
                    modbusPort->state = MB_PORT_STATE_MASTER_IDLE;
                    break; 
                }         
                break;
            }
            case MB_PORT_STATE_MASTER_RX_PROCESSING:
            {
                // Timeout happened, check what was received so far
                if(modbusPort->rx_buffer_length == 0)
                {
                    DEBUG_LOGW(DEBUG_TAG_MODBUS, "MB", "Response timeout from slave");
                    slaveInProcess->status.connected = 0; // Mark the slave as disconnected
                    modbusPort->state = MB_PORT_STATE_MASTER_RESET;
                }
                else
                {	
                    slaveResponseTimer = 0; // Reset the timer as a response was received
                    //mbPhyPreRx(modbusPort, modbusPort->rx_buffer, byte);
                    mbRxFrameParse(modbusPort, 
                       queryInProcess.slaveId,
                        modbusPort->tx_buffer[1], 
                        queryInProcess.address, 
                        queryInProcess.regCount);
                    modbusPort->state = MB_PORT_STATE_MASTER_RX_PROCESSING;
                }  
                slaveInProcess->status.connected = 1; // Mark the slave as connected
                modbusPort->state = MB_PORT_STATE_MASTER_RESET;
                break;
            }
            case MB_PORT_STATE_MASTER_RESET:
            {
                // Reset the Modbus port state
                slaveResponseTimer = 0;
                interFrameDelayTimer = 0;
                configRqst.rqstdata = NULL;
                configRqst.rqstType = _MB_MASTER_RQST_TYPE_MAX;
                queryInProcess.slaveId = 0;
                queryInProcess.address = 0;
                queryInProcess.regCount = 0;
                slaveInProcess = NULL;
                modbusPort->state = MB_PORT_STATE_MASTER_IDLE; // Reset to idle state

                xQueueReset (ctx->rxQueueHandle);
				modbusPort->rx_buffer_length = 0;

                memset(modbusPort->rx_buffer, 0, sizeof(modbusPort->rx_buffer));
                memset(modbusPort->tx_buffer, 0, sizeof(modbusPort->tx_buffer));
                modbusPort->rx_buffer_length = 0;
                modbusPort->tx_buffer_length = 0;
                
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
    vTaskDelete(NULL); // Exit the task when done
}


// create master event group
gsg_result_t MB_masterRegisterSlave(modbus_port_t *port, modbus_slave_info_t * slave, uint8_t maxEvents )
{
    uint8_t i = 0;

    // Check if the slave ID already exists
    for (i = 0; i < MODBUS_MASTER_MAX_SLAVES; i++)
    {
        if (port->slave[i]->id == slave->id)
            return GSG_INVALID_ARG; // Slave ID already exists
    }

    // Check if there is a free slot in slave ptr list
    for (i = 0; i < MODBUS_MASTER_MAX_SLAVES; i++)
    {
        if (port->slave[i] == NULL)
        {
            // Found a free slot
            port->slave[i] = slave;
            break;
        }
    }
    if(i == MODBUS_MASTER_MAX_SLAVES)
        return GSG_OVERFLOW;

    // Register the new slave information
    port->slave[i]->status.connected            = 0; // Initialize status to 0
    
    if(maxEvents && (port->slave[i]->eventQueueHandle == NULL))
        port->slave[i]->eventQueueHandle = xQueueCreate(maxEvents, sizeof(modbus_slave_event_t));

    #if (MODBUS_MASTER_USE_UNIFIED_REGISTER_MAP == DISABLED)

        if(slave->holdingRegistersCount == 0)
        {
            port->slave[i]->holdingRegisters = NULL; // Initialize to NULL
            port->slave[i]->holdingRegistersCount = 0; // Initialize count to 0
        }
        if(slave->inputRegistersCount == 0)
        {
            port->slave[i]->inputRegisters = NULL; // Initialize to NULL
            port->slave[i]->inputRegistersCount = 0; // Initialize count to 0
        }
        if(slave->coilsCount == 0)
       {
            port->slave[i]->coils = NULL; // Initialize to NULL
            port->slave[i]->coilsCount = 0; // Initialize count to 0
        }
        if(slave->discreteInputsCount == 0)
        {
            port->slave[i]->discreteInputs = NULL; // Initialize to NULL
            port->slave[i]->discreteInputsCount = 0; // Initialize count to 0
        }
    #else
        #error "Unified Register map not implemented yet"
    #endif

    return GSG_SUCCESS;
}

gsg_result_t MB_masterUnregisterSlave(modbus_port_t *port, uint8_t slaveId )
{
    uint8_t i = 0;
    for (i = 0; i < MODBUS_MASTER_MAX_SLAVES; i++)
    {
        if (port->slave[i] == NULL)
            continue;

        if (port->slave[i]->id == slaveId)
        {
            // Delete event queue for the slave
            if (port->slave[i]->eventQueueHandle != NULL)
            {
                vQueueDelete(port->slave[i]->eventQueueHandle);
                port->slave[i]->eventQueueHandle = NULL;
            }
            
            port->slave[i] = NULL;
            #if (MODBUS_MASTER_USE_UNIFIED_REGISTER_MAP == DISABLED)
                ;
            #else
                #error "Unified Register map not implemented yet"
            #endif

            return GSG_SUCCESS;
        }
    }
    return GSG_INVALID_ARG; // Slave ID not found
}

void mbMasterQueryTimerHandler( TimerHandle_t xTimer )
{
    modbus_port_t *port = (modbus_port_t *) pvTimerGetTimerID(xTimer);
    DEBUG_ASSERT(port != NULL);
    
    uint32_t currentTime = xTaskGetTickCount();  // Get current time in ms
    // return;
    for (uint8_t i = 0; i < MODBUS_MASTER_QUERY_LIST_LENGTH; i++)
    {
        mb_master_query_t *query = port->queryList[i];
        if (query == NULL)
            continue;
        
        uint32_t elapsed = currentTime - query->lastExecutionTimeMs;
        uint32_t periodMs = 0;

        switch (query->periodicity)
        {
            case MB_MASTER_QUERY_APERIODIC:     periodMs = 0;       break;
            case MB_MASTER_QUERY_PERIOD_100_MS: periodMs = 100;     break;
            case MB_MASTER_QUERY_PERIOD_200_MS: periodMs = 200;     break;
            case MB_MASTER_QUERY_PERIOD_500_MS: periodMs = 500;     break;
            case MB_MASTER_QUERY_PERIOD_1_S:    periodMs = 1000;    break;
            case MB_MASTER_QUERY_PERIOD_2_S:    periodMs = 2000;    break;
            case MB_MASTER_QUERY_PERIOD_5_S:    periodMs = 5000;    break;
            case MB_MASTER_QUERY_PERIOD_10_S:   periodMs = 10000;   break;
            case MB_MASTER_QUERY_PERIOD_30_S:   periodMs = 30000;   break;
            case MB_MASTER_QUERY_PERIOD_1_MIN:  periodMs = 60000;   break;
            case MB_MASTER_QUERY_PERIOD_5_MIN:  periodMs = 300000;  break;
            case MB_MASTER_QUERY_PERIOD_10_MIN: periodMs = 600000;  break;
            case MB_MASTER_QUERY_PERIOD_30_MIN: periodMs = 1800000; break;
            case MB_MASTER_QUERY_PERIOD_1_HOUR: periodMs = 3600000; break;
            case MB_MASTER_QUERY_DISABLED:  // Skip disabled queries
            default:  // Skip unrecognized types
                continue;              
        }

        if ((elapsed >= periodMs) || (periodMs == 0))
        {
            query->lastExecutionTimeMs = currentTime;
            if (xQueueSend(port->queryQueueHandle, &query, 0) != pdTRUE)
            {
                DEBUG_LOGW(DEBUG_TAG_MODBUS,"MBM","Query queue full");
            }

            // Remove the query if it's aperiodic, freeing up a slot
            if(query->periodicity == MB_MASTER_QUERY_APERIODIC)
            {	
                MB_masterUnregisterQuery(port, query);
            }
        }
    }
}

void mb_masterProcessConfigRequest(modbus_port_t *port, mb_config_request_t *configRqst)
{
    // Process the configuration request

    switch (configRqst->rqstType)
    {
        case MB_MASTER_ADD_QUERY:
        {
            mb_master_query_t *query = NULL;
            query = (mb_master_query_t *)configRqst->rqstdata;
            if(query == NULL)
                return;
            // Stop the port's query handler timer before accessing
            xTimerStop(port->queryTimerHandle,0);

            for (uint8_t i = 0; i < MODBUS_MASTER_QUERY_LIST_LENGTH; i++)
            {
                // check for a free slot in the list and save the query
                if (port->queryList[i] == NULL)
                {   
                    // DEBUG_LOGD(DEBUG_TAG_MODBUS,"MBM","Query Added");
                    port->queryList[i] = query;
                    break;
                }
            }
            // Start the query timer back
            xTimerStart(port->queryTimerHandle, 0);
            break;
        }
        case MB_MASTER_REMOVE_QUERY:
        {
            mb_master_query_t *query = (mb_master_query_t *)configRqst->rqstdata;
            if(query == NULL)
                return;

            // Stop the port's query handler timer before accessing
            xTimerStop(port->queryTimerHandle,0);

            for (uint8_t i = 0; i < MODBUS_MASTER_QUERY_LIST_LENGTH; i++)
            {
                // check for the query in the list and remove it
                if (port->queryList[i] == query)
                {   
                    // DEBUG_LOGD(DEBUG_TAG_MODBUS,"MBM","Query Removed");
                    port->queryList[i] = NULL;
                    break;
                }
            }
            // Start the query timer back
            xTimerStart(port->queryTimerHandle, 0);
            break;
        }
        case MB_MASTER_UPDATE_QUERY:
        {
            mb_master_query_t *query = (mb_master_query_t *)configRqst->rqstdata;
            if(query == NULL)
                return;

            // Stop the port's query handler timer before accessing
            xTimerStop(port->queryTimerHandle,0);

            for (uint8_t i = 0; i < MODBUS_MASTER_QUERY_LIST_LENGTH; i++)
            {
                // check for the query in the list and update it
                if (port->queryList[i] == query)
                {
                    DEBUG_LOGD(DEBUG_TAG_MODBUS,"MBM","Query Updated");
                    port->queryList[i] = query;
                    break;
                }
            }
            // Start the query timer back
            xTimerStart(port->queryTimerHandle, 0);
            break;
        }
        case MB_MASTER_ADD_SLAVE:
        {
            DEBUG_LOGD(DEBUG_TAG_MODBUS,"MBM","Slave Added");
            break;
        }
        case MB_MASTER_REMOVE_SLAVE:
        {
            DEBUG_LOGD(DEBUG_TAG_MODBUS,"MBM","Slave Removed");
            break;
        }
        case MB_MASTER_UPDATE_SLAVE:
        {
            DEBUG_LOGD(DEBUG_TAG_MODBUS,"MBM","Slave Updated");
            break;
        }
        default:
            break;
    }
}

gsg_result_t MB_masterRegisterQuery(modbus_port_t *port, mb_master_query_t *query)
{
    mb_config_request_t request;
    request.rqstType = MB_MASTER_ADD_QUERY;
    request.rqstdata = query;

    

    // Register a config change request into the queue
    if (xQueueSend(port->configRqstQueHandle, &request, 0) != pdTRUE)
    {
    
        char *task;
        char hex[50];
        TaskHandle_t callingTask = xTaskGetCurrentTaskHandle();
        task = pcTaskGetName(callingTask);
        sprintf(hex, "[%s]CfgQueFull_r", task);
        DEBUG_LOGE(DEBUG_TAG_MODBUS,"MBM",hex);

        return GSG_OVERFLOW; // No space
    }
    return GSG_SUCCESS;
}

gsg_result_t MB_masterUnregisterQuery(modbus_port_t *port, mb_master_query_t *query)
{
    mb_config_request_t request;
    request.rqstType = MB_MASTER_REMOVE_QUERY;
    request.rqstdata = query;
    
    // Register a config change request into the queue
    if (xQueueSend(port->configRqstQueHandle, &request, 0) != pdTRUE)
    {
        char *task;
        TaskHandle_t callingTask = xTaskGetCurrentTaskHandle();
        task = pcTaskGetName(callingTask);

        DEBUG_LOGE(DEBUG_TAG_MODBUS,task,"Modbus new request queue full [uq]");

        return GSG_OVERFLOW; // No space
    }
    return GSG_NOT_FOUND; // Query not found
}

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
