
#include "serial.h"

gsg_result_t SER_openPort( serial_port_t *port, serial_port_type_t type, uint16_t txBuffSize, uint16_t rxBuffSize, uint8_t flags, uint8_t txEnPin, uint8_t rxEnPin, void *peripheralHandle)
{
    configASSERT(port != NULL);
    configASSERT(type < __SERIAL_PORT_TYPE_MAX);
    configASSERT(peripheralHandle != NULL);

    if(port->state != SERIAL_PORT_INVALID)  
        return GSG_ERROR;   // Port already open or in invalid state

    if(port->sendData == NULL)
        return GSG_ERROR;   // Port already open or in invalid state
    
    #if (GSG_OS_USED == GSG_OS_FREERTOS)	//freeRTOS
    port->ownershipMutex = xSemaphoreCreateMutex();
    #endif

    port->context = peripheralHandle;

    port->txHead        = 0;
	port->txTail        = 0;
	port->txCount       = 0;
	port->txBuffSize    = 0;
    if(txBuffSize > 0) 
    {
        if(flags & SERIAL_USE_STATIC_TX_BUFFER)
        {
            configASSERT(port->txBuffer != NULL);
        }
        else
        {
            port->txBuffer = calloc(txBuffSize, sizeof(uint8_t));
        }
        port->txBuffSize = txBuffSize;
        #if (GSG_OS_USED == GSG_OS_FREERTOS)	//freeRTOS
        port->txBufferMutex = xSemaphoreCreateMutex();
        #endif
    }
    
    port->rxHead        = 0;
    port->rxTail        = 0;
    port->rxCount       = 0;
    port->rxBuffSize    = 0;
    if(rxBuffSize > 0) 
    {
        if(flags & SERIAL_USE_STATIC_RX_BUFFER)
        {
            configASSERT(port->rxBuffer != NULL);
        }
        else
        {
            port->rxBuffer = calloc(rxBuffSize, sizeof(uint8_t));
        }
        port->rxBuffSize = rxBuffSize;
        #if (GSG_OS_USED == GSG_OS_FREERTOS)	//freeRTOS
        port->rxBufferMutex = xSemaphoreCreateMutex();
        port->rxDataCountSema = xSemaphoreCreateCounting(rxBuffSize, 0);
        #endif
    }

    #if (GSG_OS_USED == GSG_OS_FREERTOS)	//freeRTOS
    if((port->txBufferMutex == NULL && port->txBuffSize > 0) ||
        (port->rxBufferMutex == NULL && port->rxBuffSize > 0) ||
        (port->rxDataCountSema == NULL && port->rxBuffSize > 0) ||
         port->ownershipMutex == NULL)
    {
        SER_closePort(port);
        return GSG_ERROR;
    }
    #endif

   if(type == SERIAL_PORT_RS485)
    {
        port->txEnPin = txEnPin;
        port->rxEnPin = rxEnPin;
    }

    port->type  = type;
    port->flags = flags;
    // Not yet ready to use until handlers are registered
    port->state = SERIAL_PORT_INVALID;
	return GSG_OK;
}

gsg_result_t SER_registerHandlers(serial_port_t *port, serial_tx_fn_t txHandler, serial_rx_fn_t rxHandler)
{
    configASSERT(port != NULL);
    
    if(txHandler != NULL)
    {
        port->sendData = txHandler;
    }
    if(rxHandler != NULL)
    {
        port->receiveData = rxHandler;
    }
    return GSG_SUCCESS;
}

gsg_result_t SER_closePort(serial_port_t *port)
{
    configASSERT(port != NULL);

    if(port->rxBufferMutex != NULL)
        vSemaphoreDelete(port->rxBufferMutex);
    if(port->txBufferMutex != NULL)
        vSemaphoreDelete(port->txBufferMutex);
    if(port->ownershipMutex != NULL)
        vSemaphoreDelete(port->ownershipMutex);
    if(port->rxDataCountSema != NULL)
        vSemaphoreDelete(port->rxDataCountSema);

    if(!(port->flags & SERIAL_USE_STATIC_TX_BUFFER))
        free(port->txBuffer);
    if(!(port->flags & SERIAL_USE_STATIC_RX_BUFFER))
        free(port->rxBuffer);

    port->ownershipMutex    = NULL;
    port->state             = SERIAL_PORT_INVALID;
    port->type              = SERIAL_PORT_UART;
    port->flags             = 0;
    port->context           = NULL;
    
    port->txBuffer          = NULL;
    port->txBufferMutex     = NULL;
    port->sendData          = NULL;
    port->txHead            = 0;
	port->txTail            = 0;
	port->txCount           = 0;
	port->txBuffSize        = 0;
    
    port->rxBuffer          = NULL;
    port->rxBufferMutex     = NULL;
    port->receiveData       = NULL;
    port->rxDataCountSema   = NULL;
    port->rxHead            = 0;
	port->rxTail            = 0;
	port->rxCount           = 0;
	port->rxBuffSize        = 0;
    
    return GSG_OK;
}

gsg_result_t SER_acquirePort(serial_port_t *port, uint32_t timeout)
{
    configASSERT(port != NULL);

    if(xSemaphoreTake(port->ownershipMutex, pdMS_TO_TICKS(timeout)) != pdTRUE)
    {
        return GSG_TIMEOUT;
    }

    port->ownerTask = xTaskGetCurrentTaskHandle();

    return GSG_SUCCESS;
}

gsg_result_t SER_getPortOwner(serial_port_t *port, TaskHandle_t *ownerTask)
{
    configASSERT(port != NULL);
    configASSERT(ownerTask != NULL);

    if(xSemaphoreTake(port->ownershipMutex, pdMS_TO_TICKS(100)) != pdTRUE)
    {
        return GSG_TIMEOUT;
    }

    *ownerTask = port->ownerTask;

    xSemaphoreGive(port->ownershipMutex);

    return GSG_SUCCESS;
}

gsg_result_t SER_releasePort(serial_port_t *port)
{
    configASSERT(port != NULL);

    if(port->ownerTask != xTaskGetCurrentTaskHandle())
    {
        return GSG_ERROR;
    }

    port->ownerTask = NULL;

    xSemaphoreGive(port->ownershipMutex);

    return GSG_SUCCESS;
}


gsg_result_t SER_sendData(serial_port_t *port, uint8_t *data, uint16_t length, uint16_t timeout)
{
    configASSERT(port != NULL);
    configASSERT(data != NULL);
    configASSERT(port->sendData != NULL);
    if(port->ownerTask != xTaskGetCurrentTaskHandle())
        return GSG_BUSY;

    return port->sendData(port->context, data, length, timeout);
}

static void _popDataFromRxBuffer(serial_port_t *port, uint8_t *data, uint16_t length)
{
    if(port == NULL || data == NULL || length == 0)
        return;

    xSemaphoreTake(port->rxBufferMutex, portMAX_DELAY);

    for(uint16_t i = 0; i < length; i++)
    {
        if(port->rxCount == 0)
            break;

        data[i] = port->rxBuffer[port->rxTail];

        port->rxTail = (port->rxTail + 1) % port->rxBuffSize;

        port->rxCount--;
    }

    xSemaphoreGive(port->rxBufferMutex);
}

static void _pushByteIntoRxBuffer(serial_port_t *port, uint8_t byte)
{
    BaseType_t higherTaskWoken = pdFALSE;
    uint16_t nextHead;

    if(port == NULL || port->rxBuffer == NULL)
        return;

    nextHead = (port->rxHead + 1) % port->rxBuffSize;

    if(nextHead == port->rxTail)
        return;

    port->rxBuffer[port->rxHead] = byte;
    port->rxHead = nextHead;

    if(port->rxCount < port->rxBuffSize)
        port->rxCount++;

    xSemaphoreGiveFromISR(port->rxDataCountSema, &higherTaskWoken);

    portYIELD_FROM_ISR(higherTaskWoken);
} 

gsg_result_t SER_receiveData(serial_port_t *port, uint8_t *data, uint16_t requestedLength, uint16_t *receivedLength, uint32_t timeout)
{
    uint16_t count = 0;

    configASSERT(port != NULL);

    *receivedLength = 0;

    if(port->ownerTask != xTaskGetCurrentTaskHandle())
        return GSG_BUSY;

    while(count < requestedLength)
    {
        if(xSemaphoreTake(port->rxDataCountSema, pdMS_TO_TICKS(timeout)) != pdTRUE)
            break;
        uint16_t before = port->rxCount;
        _popDataFromRxBuffer(port, &data[count], 1);
        if(before != port->rxCount)
            count++;
    }

    *receivedLength = count;

    return ((count > 0) ? GSG_SUCCESS : GSG_TIMEOUT);
}

void SER_rxByteIsrCallback(void *ctx, uint8_t byte)
{
    serial_port_t *port = (serial_port_t *)ctx;

    _pushByteIntoRxBuffer(port, byte);
}
