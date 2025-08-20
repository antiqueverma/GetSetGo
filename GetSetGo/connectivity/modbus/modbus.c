#include "modbus.h"
#include <include/projdefs.h>
// Based on FreeRTOS

// Private variables and function prototypes 


// Private function prototypes
void mbMasterPushQueryTimerCallback(void *arg);
void mbMasterQueryTimerHandler( TimerHandle_t xTimer );

void modbusMasterTaskHandler(void * argument);
void modbusSlaveTaskHandler(void * argument);


gsg_result_t MB_createPortStatic(modbus_port_t * port)
{
    DEBUG_ASSERT(port != NULL);
    DEBUG_ASSERT(port->mode != MODBUS_MODE_NONE);
    DEBUG_ASSERT(port->mode <= _MODBUS_MODE_MAX);
    DEBUG_ASSERT(port->phy != MODBUS_PHY_NONE);
    DEBUG_ASSERT(port->phy < _MODBUS_PHY_MAX);

    port->state = MB_PORT_STATE_DISABLED; // Set initial state to disabled
    for (uint8_t i = 0; i < MODBUS_MASTER_QUERY_LIST_LENGTH; i++)
	{
    	port->queryList[i] = NULL;
	}
    // Reset memory for the port buffers
    memset(port->rx_buffer, 0, MODBUS_PORT_RX_BUFFER_SIZE);
    memset(port->tx_buffer, 0, MODBUS_PORT_TX_BUFFER_SIZE);

    port->rx_buffer_length = port->tx_buffer_length = 0;

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
    DEBUG_ASSERT(port != NULL);
    DEBUG_ASSERT(port->state == MB_PORT_STATE_DISABLED);

    // Create event flags
    // port->eventHandle = osEventFlagsNew(NULL);
    // if (port->eventHandle == NULL)
    //     return GSG_ERROR;


    return GSG_ERROR;
    #if (MODBUS_MASTER_MODE == ENABLED)
        if(port->mode == MODBUS_MODE_MASTER)
        {
            port->slaveData = NULL; // No slave info in master mode

            /*Create query queue (FIFO for pending queries)*/
             port->queryQueueHandle = xQueueCreate(MODBUS_MASTER_QUERY_QUEUE_LENGTH, sizeof(mb_master_query_t *));
            DEBUG_ASSERT(port->queryQueueHandle != NULL);

            /*Create configuration request queue*/
		 	port->configRqstQueHandle = xQueueCreate(MODBUS_CONFIG_REQUEST_QUEUE_LENGTH, sizeof(mb_config_request_t));
		 	DEBUG_ASSERT(port->configRqstQueHandle != NULL);

			// Start periodic query timer and save the handle
//			port->queryTimerHandle = xTimerCreate("MBMQT",
//                 pdMS_TO_TICKS(100),
//                 pdTRUE,
//				 (void *) port,
//                 mbMasterQueryTimerHandler);

			 DEBUG_ASSERT(port->queryTimerHandle != NULL);
			 xTimerStart(port->queryTimerHandle, 0);
        }
    #endif

    // Start Modbus task
    if(port->mode == MODBUS_MODE_MASTER)
    {
         xTaskCreate(modbusMasterTaskHandler,
             "MBMTask",
             MODBUS_TASK_STACK_SIZE,
             port,
             MODBUS_TASK_PRIORITY,
             &port->taskHandle);
    }
    else if(port->mode == MODBUS_MODE_SLAVE)
    {
        xTaskCreate(modbusSlaveTaskHandler, 
            "MBSTask", 
            MODBUS_TASK_STACK_SIZE, 
            port, 
            MODBUS_TASK_PRIORITY, 
            &port->taskHandle);
    }
    
     DEBUG_ASSERT(port->taskHandle != NULL);
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
        vTaskDelete(port->taskHandle);
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

