
#ifndef SPI_H_
#define SPI_H_

#include <stdint.h>
#include <stdbool.h>
#include "FreeRTOS.h"
#include "semphr.h"
#include "gsg_defs.h"
#include "drv/win/GPIO/gpio.h"

typedef uint16_t SPI_HandleTypeDef;

typedef struct {
    SPI_HandleTypeDef *spiHandle;
    SemaphoreHandle_t spiMutex;
    gpio_pin_t csPin;
    spi_mode_t mode;
    bool csPinActiveLow;
} spi_port_t;

gsg_result_t SPI_Init(spi_port_t *spi, SPI_HandleTypeDef *spiInstance, gpio_pin_t csPin, bool csPinActiveLow);
gsg_result_t SPI_DeInit(spi_port_t *spi);
gsg_result_t SPI_writeData(void *spiCtx, const uint8_t *data, uint16_t size, uint32_t timeout);
gsg_result_t SPI_readData(void *spiCtx, uint8_t *data, uint16_t size, uint32_t timeout);
gsg_result_t SPI_transferData(void *spiCtx, const uint8_t *txData, uint8_t *rxData, uint16_t size, uint32_t timeout);

#endif /* SPI_H_ */ 
