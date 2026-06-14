
#include <stdint.h>
#include <stdbool.h>
#include "spi.h"

static bool _spi_getLock(spi_port_t *spi, uint32_t timeout)
{
    if (spi == NULL || spi->spiMutex == NULL)
        return false;

    return (xSemaphoreTake(spi->spiMutex, pdMS_TO_TICKS(timeout)) == pdTRUE);
}

static bool _spi_releaseLock(spi_port_t *spi)
{
    if (spi == NULL || spi->spiMutex == NULL)
        return false;

    return (xSemaphoreGive(spi->spiMutex) == pdTRUE);
}

static void _spi_assertCS(spi_port_t *spi)
{
    if (spi == NULL)
        return;

    /* Toggle CS according to active level */
    gpio_pin_state_t state = spi->csPinActiveLow ? GPIO_PIN_RESET : GPIO_PIN_SET;
    (void)GPIO_writePin(spi->csPin, state);
}

static void _spi_deassertCS(spi_port_t *spi)
{
    if (spi == NULL)
        return;

    gpio_pin_state_t state = spi->csPinActiveLow ? GPIO_PIN_SET : GPIO_PIN_RESET;
    (void)GPIO_writePin(spi->csPin, state);
}

gsg_result_t SPI_Init(spi_port_t *spi, SPI_HandleTypeDef *spiInstance, gpio_pin_t csPin, bool csPinActiveLow)
{
    if((spi == NULL) || (spiInstance == NULL))
        return GSG_INVALID_ARG;

    spi->spiHandle = spiInstance;
    spi->csPin = csPin;
    spi->csPinActiveLow = csPinActiveLow;

    spi->spiMutex = xSemaphoreCreateMutex();
    if(spi->spiMutex == NULL)
    {
        spi->spiHandle = NULL;
        return GSG_ERROR;
    }

    return GSG_SUCCESS;
}

gsg_result_t SPI_DeInit(spi_port_t *spi)
{
    if (spi == NULL)
        return GSG_INVALID_ARG;

    if (spi->spiMutex != NULL)
    {
        vSemaphoreDelete(spi->spiMutex);
        spi->spiMutex = NULL;
    }

    spi->spiHandle = NULL;
    return GSG_SUCCESS;
}

gsg_result_t SPI_writeData(void *spiCtx, const uint8_t *data, uint16_t size, uint32_t timeout)
{
    spi_port_t *spi = (spi_port_t *)spiCtx;
    if(spi == NULL || data == NULL)
        return GSG_INVALID_ARG;

    if(_spi_getLock(spi, timeout))
    {
        gsg_result_t result = GSG_ERROR;
        _spi_assertCS(spi);
        if(HAL_SPI_Transmit(spi->spiHandle, (uint8_t *)data, size, timeout) == HAL_OK)
        {
            result = GSG_SUCCESS;
        }
        _spi_deassertCS(spi);
        _spi_releaseLock(spi);
        return result;
    }
    else
    {
        return GSG_ERROR;
    }
}

gsg_result_t SPI_readData(void *spiCtx, uint8_t *data, uint16_t size, uint32_t timeout)
{
    spi_port_t *spi = (spi_port_t *)spiCtx;
    if(spi == NULL || data == NULL)
        return GSG_INVALID_ARG;

    if(_spi_getLock(spi, timeout))
    {
        gsg_result_t result = GSG_ERROR;
        _spi_assertCS(spi);
        if(HAL_SPI_Receive(spi->spiHandle, data, size, timeout) == HAL_OK)
        {
            result = GSG_SUCCESS;
        }
        _spi_deassertCS(spi);
        _spi_releaseLock(spi);
        return result;
    }
    else
    {
        return GSG_ERROR;
    }
}

gsg_result_t SPI_transferData(void *spiCtx, const uint8_t *txData, uint8_t *rxData, uint16_t size, uint32_t timeout)
{
    spi_port_t *spi = (spi_port_t *)spiCtx;
    if(spi == NULL || txData == NULL || rxData == NULL)
        return GSG_INVALID_ARG;

    if(_spi_getLock(spi, timeout))
    {
        gsg_result_t result = GSG_ERROR;
        _spi_assertCS(spi);
        if(HAL_SPI_TransmitReceive(spi->spiHandle, (uint8_t *)txData, rxData, size, timeout) == HAL_OK)
        {
            result = GSG_SUCCESS;
        }
        _spi_deassertCS(spi);
        _spi_releaseLock(spi);
        return result;
    }
    else
    {
        return GSG_ERROR;
    }
}
