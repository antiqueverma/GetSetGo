
#ifndef I2C_H_
#define I2C_H_

#include <stdint.h>
#include "stm32f4xx_hal.h"
#include "FreeRTOS.h"
#include "semphr.h"
#include "queue.h"
#include "gsg_defs.h"


typedef struct {
    SemaphoreHandle_t mutex;
    I2C_HandleTypeDef *i2cHandle;
    uint16_t speedHz;
} i2c_port_t;

gsg_result_t I2C_Init(i2c_port_t *i2c, I2C_HandleTypeDef *i2cInstance);
gsg_result_t I2C_WriteData(void * i2cCtx, uint16_t devAddress, uint8_t *data, uint16_t size, uint32_t timeout);
gsg_result_t I2C_ReadData(void * i2cCtx, uint16_t devAddress, uint8_t *data, uint16_t size, uint32_t timeout);
gsg_result_t I2C_WriteMemory16(void *i2cCtx, uint16_t devAddress, uint16_t regAddress, uint8_t *data, uint16_t size, uint32_t timeout);
gsg_result_t I2C_ReadMemory16(void *i2cCtx, uint16_t devAddress, uint16_t regAddress, uint8_t *data, uint16_t size, uint32_t timeout);
gsg_result_t I2C_WriteMemory8(void *i2cCtx, uint16_t devAddress, uint8_t regAddress, uint8_t *data, uint16_t size, uint32_t timeout);
gsg_result_t I2C_ReadMemory8(void *i2cCtx, uint16_t devAddress, uint8_t regAddress, uint8_t *data, uint16_t size, uint32_t timeout);
gsg_result_t I2C_IsDeviceReady(void *i2cCtx, uint16_t devAddress, uint32_t trials, uint32_t timeout);
gsg_result_t I2C_DeInit(i2c_port_t *i2c);


#endif /* I2C_H_ */
