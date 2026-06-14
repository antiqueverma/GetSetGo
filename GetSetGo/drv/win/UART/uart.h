
#ifndef UART_H_
#define UART_H_

#include <stdint.h>
#include "stm32f4xx_hal.h"
#include "FreeRTOS.h"
#include "semphr.h"
#include "port/port.h"
#include "gsg_defs.h"

typedef void (*uartRxCallback)(void *ctx, uint8_t byte);

typedef struct {
    SemaphoreHandle_t mutex;
    SemaphoreHandle_t txCpltSema;
    UART_HandleTypeDef *uartHandle;
    uartRxCallback rxCallback;
    uint8_t rxTempByte;
} uart_port_t;

gsg_result_t UART_Init(uart_port_t *uart, UART_HandleTypeDef *uartInstance);
gsg_result_t UART_deInit(uart_port_t *uart);
gsg_result_t UART_writeData(void *uartCtx, uint8_t *data, uint16_t size, uint16_t timeout);


#endif /* UART_H_ */
