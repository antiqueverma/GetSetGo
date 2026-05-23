
#include "serial.h"
#include "main.h"

gsg_result_t SER_openPort( serial_port_t *port, serial_port_type_t type, uint16_t txBuffSize, serial_tx_fn_t tx_fn, uint16_t rxBuffSize)
{
    DEBUG_ASSERT(port != NULL);
    if(txBuffSize > 0 )
    {
        port->txQueue = xQueueCreate(txBuffSize, sizeof(uint8_t));
    }

    if(rxBuffSize > 0 )
    {
        port->rxQueue = xQueueCreate(rxBuffSize, sizeof(uint8_t));
    }

    port->tx_fn = tx_fn;
    port->type  = type;

	return GSG_OK;
}

gsg_result_t SER_closePort(serial_port_t *port)
{
    DEBUG_ASSERT(port != NULL);

    if (port->rxQueue != NULL)
    {
        vQueueDelete(port->rxQueue);
        port->rxQueue = NULL;
    }
    if (port->txQueue != NULL)
    {
        vQueueDelete(port->txQueue);
        port->txQueue = NULL;
    }
    return GSG_OK;
}

// Function to be called from MCU specific ISR
void SER_rxByteISRcb(serial_port_t *port, uint8_t byteReceived)
{
    configASSERT(port != NULL);
    configASSERT(port->rxQueue != NULL);

    xQueueSendFromISR(port->rxQueue, &byteReceived, NULL);
}

// Function to be called from Application to get n bytes from serial rx buffer
uint16_t SER_receiveData(serial_port_t *port, uint8_t *data, uint16_t length, uint16_t timeout)
{
    DEBUG_ASSERT(port != NULL);
    DEBUG_ASSERT(data != NULL);
    DEBUG_ASSERT(length != 0);
    DEBUG_LOGI(DEBUG_TAG_ESP,"ESP","#C");
    port->state = SERIAL_PORT_RECEIVING;

    uint16_t bytesRead = 0;
    while (bytesRead < length)
    {   DEBUG_LOGI(DEBUG_TAG_ESP,"ESP","#D");

        if (xQueueReceive(port->rxQueue, &data[bytesRead], pdMS_TO_TICKS(timeout)) == pdTRUE)
        {

            DEBUG_LOGI(DEBUG_TAG_ESP,"ESP","#E");
            bytesRead++;
        }
        else
        {

			#error "Last problem here, the queue is not giving back any data"
        	configASSERT(0);
            break;
        }
    }

    return bytesRead;
}

// Function to be called from Application to send n bytes
void SER_sendData(serial_port_t *port, uint8_t *data, uint16_t length, uint16_t timeout)
{
    DEBUG_ASSERT(port != NULL);
    DEBUG_ASSERT(data != NULL);
    DEBUG_ASSERT(length != 0);

    port->state = SERIAL_PORT_SENDING;

    if(port->txQueue != NULL)
    {
        for (uint16_t i = 0; i < length; i++)
        {
            xQueueSend(port->txQueue, &data[i], pdMS_TO_TICKS(timeout));
        }
    }
    else if(port->tx_fn != NULL)
    {
        port->tx_fn(data, length, timeout);
    }
}

void SER_sendString(serial_port_t *port, char *data, uint16_t timeout)
{
    DEBUG_ASSERT(port != NULL);
    DEBUG_ASSERT(data != NULL);

    port->state = SERIAL_PORT_SENDING;
    
    uint16_t length = strlen((const char *)data);

    // if(port->txQueue != NULL)
    // {
    //     for (uint16_t i = 0; i < length; i++)
    //     {
    //         xQueueSend(port->txQueue, &data[i], pdMS_TO_TICKS(timeout));
    //     }
    // }
    // else 
    if(port->tx_fn != NULL)
    {
        port->tx_fn((uint8_t *)data, length, timeout);
    }
}

void SER_flushRxBuffer(serial_port_t *port)
{
    DEBUG_ASSERT(port != NULL);
    if (port->rxQueue != NULL)
    {
        xQueueReset(port->rxQueue);
    }
}

void SER_flushTxBuffer(serial_port_t *port)
{
    DEBUG_ASSERT(port != NULL);
    if (port->txQueue != NULL)
    {
        xQueueReset(port->txQueue);
    }
}


































