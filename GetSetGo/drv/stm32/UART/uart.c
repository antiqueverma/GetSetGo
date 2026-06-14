
#include "uart.h"

static uart_port_t *uartPorts[PORT_PERIPHERAL_UART_COUNT];

// HAL based generic uart wrappers

static bool _uart_getLock(uart_port_t *uart, uint32_t timeout)
{
    configASSERT(uart != NULL);
    configASSERT(uart->mutex != NULL);

    return (xSemaphoreTake(uart->mutex, pdMS_TO_TICKS(timeout)) == pdTRUE);
}

static bool _uart_releaseLock(uart_port_t *uart)
{
    configASSERT(uart != NULL);
    configASSERT(uart->mutex != NULL);

    return (xSemaphoreGive(uart->mutex) == pdTRUE);
}

static gsg_result_t _uart_halStatusToResult(HAL_StatusTypeDef status)
{
    switch (status)
    {
        case HAL_OK:
            return GSG_SUCCESS;
        case HAL_BUSY:
            return GSG_BUSY;
        case HAL_TIMEOUT:
            return GSG_TIMEOUT;
        case HAL_ERROR:
        default:
            return GSG_ERROR;
    }
}

gsg_result_t UART_Init(uart_port_t *uart, UART_HandleTypeDef *uartInstance)
{
    uint8_t i = 0;
    configASSERT(uart != NULL);
    configASSERT(uartInstance != NULL);

    uart->uartHandle = uartInstance;

    uart->mutex = xSemaphoreCreateMutex();
    uart->txCpltSema = xSemaphoreCreateBinary();
    
    for(i = 0; i < PORT_PERIPHERAL_UART_COUNT; i++)
    {
        if(uartPorts[i] == NULL)
        {
            uartPorts[i] = uart;
            break;
        }
    }

    if(i == PORT_PERIPHERAL_UART_COUNT || uart->mutex == NULL || uart->txCpltSema == NULL)
    {
        if(uart->mutex != NULL)
            vSemaphoreDelete(uart->mutex);
        if(uart->txCpltSema != NULL)
            vSemaphoreDelete(uart->txCpltSema);
        uart->uartHandle = NULL;
        return GSG_ERROR;
    }

    HAL_UART_Receive_IT(uart->uartHandle, &uart->rxTempByte, 1);

    return GSG_SUCCESS;
}

gsg_result_t UART_deInit(uart_port_t *uart)
{
    configASSERT(uart != NULL);

    if(uart->mutex != NULL)
    {
        vSemaphoreDelete(uart->mutex);
    }
    if(uart->txCpltSema != NULL)
    {
        vSemaphoreDelete(uart->txCpltSema);
    }
    for(int i = 0; i < PORT_PERIPHERAL_UART_COUNT; i++)
    {
        if(uartPorts[i] == uart)
        {
            uartPorts[i] = NULL;
            break;
        }
    }

    uart->mutex = NULL;
    uart->txCpltSema = NULL;
    uart->uartHandle = NULL;
    return GSG_SUCCESS;
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    for(int i = 0; i < PORT_PERIPHERAL_UART_COUNT; i++)
    {
        if(uartPorts[i] != NULL &&
           uartPorts[i]->uartHandle == huart)
        {
            if(uartPorts[i]->rxCallback != NULL)
            {
                uartPorts[i]->rxCallback(uartPorts[i], uartPorts[i]->rxTempByte);
            }

            HAL_UART_Receive_IT(uartPorts[i]->uartHandle, &uartPorts[i]->rxTempByte, 1);

            break;
        }
    }
}

gsg_result_t UART_writeData(void *uartCtx, uint8_t *data, uint16_t size, uint16_t timeout)
{
    uart_port_t *uart = (uart_port_t *)uartCtx;

    configASSERT(uart != NULL)               ;
    configASSERT(uart->uartHandle != NULL)   ;
    configASSERT(data != NULL)               ;
    configASSERT(size != 0U)                 ;

    if(_uart_getLock(uart, timeout))
    {
        HAL_StatusTypeDef status =
            HAL_UART_Transmit_IT(uart->uartHandle, data, size);

        if(status != HAL_OK)
        {
            _uart_releaseLock(uart);
            return _uart_halStatusToResult(status);
        }

        if(xSemaphoreTake(uart->txCpltSema, pdMS_TO_TICKS(timeout)) != pdTRUE)
        {
            _uart_releaseLock(uart);
            return GSG_TIMEOUT;
        }

        _uart_releaseLock(uart);

        return GSG_SUCCESS;
    }

    return GSG_ERROR;
}

// Read will be implemented via ISR callbacks
/*
gsg_result_t UART_readData(void *uartCtx, uint8_t *data, uint16_t size, uint32_t timeout)
{
    uart_port_t *uart = (uart_port_t *)uartCtx;

    configASSERT(uart != NULL);
    configASSERT(uart->uartHandle != NULL);
    configASSERT(data != NULL);
    configASSERT(size != 0U);

    if(_uart_getLock(uart, timeout))
    {
        HAL_StatusTypeDef status = HAL_UART_Receive(uart->uartHandle, data, size, timeout);
        _uart_releaseLock(uart);
        return _uart_halStatusToResult(status);
    }

    return GSG_ERROR;
}
*/