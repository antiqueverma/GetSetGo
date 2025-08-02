#include "modbus.h"

// Private variables and function prototypes 
debugTagId_t 	debugTagId = DEBUG_TAG_COMM;
char 			debugTag[] = "MB";

static uint8_t portCount = 0; // Counter for the number of ports

// Private function prototypes
static void modbusTaskHandler(void *pvParameters);

static void modbusTaskHandler(void * argument)
{
    // typecast the port parameter to modbus_port_t pointer
    modbus_port_t *modbusPort = (modbus_port_t *)argument;
    char tempBuffer[64]; // Temporary buffer for debug messages
    modbusPort->state = MB_PORT_STATE_IDLE; // Set initial state to disabled

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
            case MB_PORT_STATE_IDLE:
            {
                if(modbusPort->mode == MODBUS_MODE_MASTER)
                {
                    modbusPort->state = MB_PORT_STATE_TX_WAITING; // Set to waiting state
                }
                else if ((modbusPort->mode == MODBUS_MODE_SLAVE) || (modbusPort->mode == MODBUS_MODE_MULTI_SLAVE))
                {
                    modbusPort->state = MB_PORT_STATE_RX_WAITING; // Set to waiting state
                }
                break;
            }
            case MB_PORT_STATE_TX_WAITING:
            {
                modbusPort->state = MB_PORT_STATE_TX_PROCESSING;
                // Wait for incoming Modbus requests
                break;
            }
            case MB_PORT_STATE_TX_PROCESSING:
            {
                modbusPort->state = MB_PORT_STATE_TRANSMITTING;
                break;
            }
            case MB_PORT_STATE_TRANSMITTING:
            {
                modbusPort->state = MB_PORT_STATE_RESET;
                break;
            }
            case MB_PORT_STATE_RX_WAITING:
            {
                modbusPort->state = MB_PORT_STATE_RX_PROCESSING; // Set to processing state
                break;
            }
            case MB_PORT_STATE_RX_PROCESSING:
            {
                modbusPort->state = MB_PORT_STATE_RESET;
                break;
            }
            case MB_PORT_STATE_RESET:
            {
                // Reset the Modbus port state
                modbusPort->state = MB_PORT_STATE_IDLE; // Reset to idle state
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
        osDelay(1000); 
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
    memset(port->rx_buffer, 0, MODBUS_PORT_RX_BUFFER_SIZE);
    memset(port->tx_buffer, 0, MODBUS_PORT_TX_BUFFER_SIZE);

    // Initialize registers, buffers, etc. as needed
    if(port->holdingRegistersCount  == 0)
        port->holdingRegisters       = NULL;

    if(port->inputRegistersCount   == 0)
        port->inputRegisters        = NULL;

    if(port->discreteInputsCount  == 0)
        port->discreteInputs       = NULL;

    if(port->coilsCount            == 0)
        port->coils                = NULL;

    return GSG_SUCCESS;

}

gsg_result_t MB_startPort(modbus_port_t * port)
{
    // Validate the input struct
    if (port == NULL)
        return GSG_INVALID_ARG;

    if(port->state != MB_PORT_STATE_DISABLED)
        return GSG_ERROR; // Port is already started or in use

    // Also pass the port struct to the task
    port->taskHandle = osThreadNew(modbusTaskHandler, port, &port->taskAttributes);

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


