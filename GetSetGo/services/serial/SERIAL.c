
#include "serial.h"

gsg_result_t SER_openPort( serialPort_t *port,
    uint16_t txBuffSize, serial_tx_fn_t tx_fn,
    uint16_t rxBuffSize)
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

	return GSG_OK;
}

gsg_result_t SER_closePort(serialPort_t *port)
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
void SER_rxByteISRcb(serialPort_t *port, uint8_t byteReceived)
{
    if(port == NULL)  return;
    if(port->rxQueue == NULL) return;

    xQueueSendFromISR(port->rxQueue, &byteReceived, NULL);
}

// Function to be called from Application to get n bytes from serial rx buffer
uint16_t SER_getRxData(serialPort_t *port, uint8_t *data, uint16_t length)
{
    DEBUG_ASSERT(port != NULL); 
    DEBUG_ASSERT(data != NULL); 
    DEBUG_ASSERT(length != 0);

    port->state = SERIAL_PORT_RECEIVING;

    uint16_t bytesRead = 0;
    while (bytesRead < length)
    {
        if (xQueueReceive(port->rxQueue, &data[bytesRead], 0) == pdTRUE)
            bytesRead++;
        else
            break;
    }

    return bytesRead;
}

// Function to be called from Application to send n bytes
void SER_sendTxdata(serialPort_t *port, uint8_t *data, uint16_t length, uint16_t timeout)
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

void SER_flushRxBuffer(serialPort_t *port)
{
    DEBUG_ASSERT(port != NULL);
    if (port->rxQueue != NULL)
    {
        xQueueReset(port->rxQueue);
    }
}

void SER_flushTxBuffer(serialPort_t *port)
{
    DEBUG_ASSERT(port != NULL);
    if (port->txQueue != NULL)
    {
        xQueueReset(port->txQueue);
    }
}
