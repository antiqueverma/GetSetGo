#include "modbus.h"

// Private variables and function prototypes 
debugTagId_t 	debugTagId = DEBUG_TAG_COMM;
char 			debugTag[] = "MB";

static uint8_t portCount = 0; // Counter for the number of ports

// Private function prototypes
void mbMasterPushQueryTimerCallback(void *arg);
void mbMasterQueryTimerHandler(void *arg);

void modbusMasterTaskHandler(void * argument);
void modbusSlaveTaskHandler(void * argument);


gsg_result_t MB_createPortStatic(modbus_port_t * port)
{
    if (port == NULL)
        return GSG_INVALID_ARG;

    if((port->mode == MODBUS_MODE_NONE) || (port->mode >= _MODBUS_MODE_MAX))
        return GSG_INVALID_ARG; // Invalid port type

    if((port->phy == MODBUS_PHY_NONE) || (port->phy >= _MODBUS_PHY_MAX))
        return GSG_INVALID_ARG; // Invalid port type

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
            port->slaveData = NULL; // No slave info in master mode

            // Create query queue (FIFO for pending queries)
            port->queryQueueHandle = osMessageQueueNew(
                MODBUS_MASTER_QUERY_QUEUE_LENGTH,             // max queue length
                sizeof(mb_master_query_t *),           // item size
                NULL
            );
            if (port->queryQueueHandle == NULL)
                return GSG_ERROR;

            // Create configuration request queue
			port->configRqstQueHandle = osMessageQueueNew(
				5,          // max queue length
				sizeof(mb_config_request_t),                // item size
				NULL
			);
			if (port->configRqstQueHandle == NULL)
				return GSG_ERROR;

			// Start periodic query timer and save the handle
			port->queryTimerHandle = osTimerNew(mbMasterQueryTimerHandler, osTimerPeriodic, port, NULL);
			if (port->queryTimerHandle == NULL)
				return GSG_ERROR;
			osTimerStart(port->queryTimerHandle, 100); // 100 ms periodic timer
        }
    #endif

    // Start Modbus task
    const osThreadAttr_t defaultTask_attributes = {
      .name = "defaultTask",
      .stack_size = 2048,
      .priority = (osPriority_t) osPriorityNormal,
    };

    if(port->mode == MODBUS_MODE_MASTER)
    {
        port->taskHandle = osThreadNew(modbusMasterTaskHandler,
    		port,
			&defaultTask_attributes);
    }
    else if(port->mode == MODBUS_MODE_SLAVE)
    {
        port->taskHandle = osThreadNew(modbusSlaveTaskHandler,
    		port,
			&defaultTask_attributes);
    }
    
    if (port->taskHandle == NULL)
    {
    	DEBUG_LOGE(DEBUG_TAG_MODBUS,"MB","Error Creating task");
        return GSG_ERROR;
    }

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

