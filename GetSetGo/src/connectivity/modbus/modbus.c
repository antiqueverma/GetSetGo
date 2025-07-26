#include "modbus.h"

#define MODBUS_PORT_RX_BUFFER_SIZE 256 // Size of the receive buffer for each Modbus port
#define MODBUS_PORT_TX_BUFFER_SIZE 256 // Size of the transmit buffer for each Modbus port
// Private variables and function prototypes 
static TaskHandle_t xTaskModbus;

static uint8_t portCount = 0; // Counter for the number of ports

static uint8_t          portRxBuffer[MODBUS_PORT_MAX_COUNT][MODBUS_PORT_RX_BUFFER_SIZE]; // Receive buffer for each port
static uint8_t          portTxBuffer[MODBUS_PORT_MAX_COUNT][MODBUS_PORT_TX_BUFFER_SIZE]; // Transmit buffer for each port
static modbus_port_t    portList[MODBUS_PORT_MAX_COUNT]; // List of Modbus ports


// Private function prototypes
static void vTaskModbus(void *pvParameters);

void MB_Init(void)
{
    portCount = 0;
    memset(portRxBuffer, 0, sizeof(portRxBuffer)); // Initialize receive buffers
    memset(portTxBuffer, 0, sizeof(portTxBuffer)); // Initialize transmit buffers

    for (uint8_t i = 0; i < MODBUS_PORT_MAX_COUNT; i++)
    {
        portList[i].type    = MODBUS_PORT_NONE; // Initialize port type
        portList[i].mode    = MODBUS_MODE_NONE; // Initialize port mode
        portList[i].phy     = MODBUS_PHY_NONE; // Initialize port physical layer
        portList[i].state   = MB_PORT_STATE_DISABLED; // Initialize port state
        portList[i].holdingRegisters = NULL; // Initialize holding registers pointer
        portList[i].inputRegisters = NULL; // Initialize input registers pointer    
        portList[i].coils = NULL; // Initialize coils pointer
        portList[i].discreteInputs = NULL; // Initialize discrete inputs pointer
        portList[i].rx_buffer = portRxBuffer[i]; // Assign receive buffer
        portList[i].tx_buffer = portTxBuffer[i]; // Assign transmit buffer
    }

    // Initialize the Modbus stack
    xTaskCreate(
        vTaskModbus,            // Task function
        "Modbus",               // Name of the task
        MODBUS_TASK_STACK_SIZE, // Stack size in words
        NULL,                   // Task parameters
        MODBUS_TASK_PRIORITY,   // Priority of the task
        &xTaskModbus            // Task handle
    );
}

uint8_t MB_registerPort(modbus_port_t *port)
{
    if(portCount > MODBUS_PORT_MAX_COUNT)   
        return 0; // Error: Maximum port count exceeded 
    if(port == NULL)
        return 0; // Error: Null port pointer
    if(port->type == MODBUS_PORT_UART || port->type == MODBUS_PORT_TCP ||
       port->type == MODBUS_PORT_RTU || port->type == MODBUS_PORT_ASCII)
    {
        ;
    }

    // Create Modbus port
    // This function would typically set up the serial port or network interface for Modbus communication.
    // For example, if using UART, you would configure the UART settings here.
}

uint8_t MB_startPort(modbus_port_t *port)
{
    // Start the Modbus port
    // This function would typically start the Modbus communication on the specified port.
    // For example, if using UART, you would enable the UART peripheral here.
}

static void vTaskModbus(void *pvParameters)
{
    // Modbus task implementation
    while (1)
    {
        
        // Process Modbus requests
        // This is where you would handle Modbus requests, read/write coils, registers, etc.    
    }

}