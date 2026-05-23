
#include "i2c.h"

// Hal based generic i2c wrappers

static bool _i2c_getLock(i2c_port_t * i2c, uint32_t timeout)
{
    if (i2c == NULL || i2c->mutex == NULL)
        return false;

    return (xSemaphoreTake(i2c->mutex, pdMS_TO_TICKS(timeout)) == pdTRUE);
}

static bool _i2c_releaseLock(i2c_port_t * i2c)
{
    if (i2c == NULL || i2c->mutex == NULL)
        return false;

    return (xSemaphoreGive(i2c->mutex) == pdTRUE);
}

gsg_result_t I2C_Init(i2c_port_t *i2c, I2C_HandleTypeDef *i2cInstance)
{
    if((i2c == NULL) || (i2cInstance == NULL))
        return GSG_INVALID_ARG;

    i2c->i2cHandle = i2cInstance;

    i2c->mutex = xSemaphoreCreateMutex();
    if(i2c->mutex == NULL)
        return GSG_ERROR;

    return GSG_SUCCESS;
}

gsg_result_t I2C_WriteData(void * i2cCtx, uint16_t devAddress, uint8_t *data, uint16_t size, uint32_t timeout)
{
    i2c_port_t *i2c = (i2c_port_t *)i2cCtx;
    if(i2c == NULL || data == NULL)
        return GSG_INVALID_ARG;

    if(_i2c_getLock(i2c, timeout))
    {
        if(HAL_I2C_Master_Transmit(i2c->i2cHandle, devAddress, data, size, timeout) == HAL_OK)
        {
            _i2c_releaseLock(i2c);
            return GSG_SUCCESS;
        }
        else
        {
            _i2c_releaseLock(i2c);
            return GSG_ERROR;
        }
    }
    else
    {
        return GSG_ERROR;
    }
}

gsg_result_t I2C_ReadData(void * i2cCtx, uint16_t devAddress, uint8_t *data, uint16_t size, uint32_t timeout)
{
    i2c_port_t *i2c = (i2c_port_t *)i2cCtx;
    if(i2c == NULL || data == NULL)
        return GSG_INVALID_ARG;

    if(_i2c_getLock(i2c, timeout))
    {
        if(HAL_I2C_Master_Receive(i2c->i2cHandle, devAddress, data, size, timeout) == HAL_OK)
        {
            _i2c_releaseLock(i2c);
            return GSG_SUCCESS;
        }
        else
        {
            _i2c_releaseLock(i2c);
            return GSG_ERROR;
        }
    }
    else
    {
        return GSG_ERROR;
    }
}

gsg_result_t I2C_WriteMemory(
    void *i2cCtx,
    uint16_t devAddress,
    uint16_t memAddress,
    uint16_t memAddressSize,
    uint8_t *data,
    uint16_t size,
    uint32_t timeout)
{
    i2c_port_t *i2c = (i2c_port_t *)i2cCtx;

    if(i2c == NULL || data == NULL)
        return GSG_INVALID_ARG;

    if(_i2c_getLock(i2c, timeout))
    {
        if(HAL_I2C_Mem_Write(
                i2c->i2cHandle,
                devAddress,
                memAddress,
                memAddressSize,
                data,
                size,
                timeout) == HAL_OK)
        {
            _i2c_releaseLock(i2c);
            return GSG_SUCCESS;
        }

        _i2c_releaseLock(i2c);
        return GSG_ERROR;
    }

    return GSG_ERROR;
}

gsg_result_t I2C_WriteMemory8(void *i2cCtx, uint16_t devAddress, uint8_t regAddress, uint8_t *data, uint16_t size, uint32_t timeout)
{
    i2c_port_t *i2c = (i2c_port_t *)i2cCtx;

    if(i2c == NULL || data == NULL)
        return GSG_INVALID_ARG;

    if(_i2c_getLock(i2c, timeout))
    {
        if(HAL_I2C_Mem_Write(
                i2c->i2cHandle,
                devAddress,
                regAddress,
                I2C_MEMADD_SIZE_8BIT,
                data,
                size,
                timeout) == HAL_OK)
        {
            _i2c_releaseLock(i2c);
            return GSG_SUCCESS;
        }

        _i2c_releaseLock(i2c);
        return GSG_ERROR;
    }

    return GSG_ERROR;
}

gsg_result_t I2C_ReadMemory8(void *i2cCtx, uint16_t devAddress, uint8_t regAddress, uint8_t *data, uint16_t size, uint32_t timeout)
{
    i2c_port_t *i2c = (i2c_port_t *)i2cCtx;

    if(i2c == NULL || data == NULL)
        return GSG_INVALID_ARG;

    if(_i2c_getLock(i2c, timeout))
    {
        if(HAL_I2C_Mem_Read(
                i2c->i2cHandle,
                devAddress,
                regAddress,
                I2C_MEMADD_SIZE_8BIT,
                data,
                size,
                timeout) == HAL_OK)
        {
            _i2c_releaseLock(i2c);
            return GSG_SUCCESS;
        }

        _i2c_releaseLock(i2c);
        return GSG_ERROR;
    }

    return GSG_ERROR;
}

gsg_result_t I2C_WriteMemory16(void *i2cCtx, uint16_t devAddress, uint16_t regAddress, uint8_t *data, uint16_t size, uint32_t timeout)
{
    i2c_port_t *i2c = (i2c_port_t *)i2cCtx;

    if(i2c == NULL || data == NULL)
        return GSG_INVALID_ARG;

    if(_i2c_getLock(i2c, timeout))
    {
        if(HAL_I2C_Mem_Write(
                i2c->i2cHandle,
                devAddress,
                regAddress,
                I2C_MEMADD_SIZE_16BIT,
                data,
                size,
                timeout) == HAL_OK)
        {
            _i2c_releaseLock(i2c);
            return GSG_SUCCESS;
        }

        _i2c_releaseLock(i2c);
        return GSG_ERROR;
    }

    return GSG_ERROR;
}

gsg_result_t I2C_ReadMemory16(void *i2cCtx, uint16_t devAddress, uint16_t regAddress, uint8_t *data, uint16_t size, uint32_t timeout)
{
    i2c_port_t *i2c = (i2c_port_t *)i2cCtx;

    if(i2c == NULL || data == NULL)
        return GSG_INVALID_ARG;

    if(_i2c_getLock(i2c, timeout))
    {
        if(HAL_I2C_Mem_Read(
                i2c->i2cHandle,
                devAddress,
                regAddress,
                I2C_MEMADD_SIZE_16BIT,
                data,
                size,
                timeout) == HAL_OK)
        {
            _i2c_releaseLock(i2c);
            return GSG_SUCCESS;
        }

        _i2c_releaseLock(i2c);
        return GSG_ERROR;
    }

    return GSG_ERROR;
}

gsg_result_t I2C_DeInit(i2c_port_t *i2c)
{
    if(i2c == NULL)
        return GSG_INVALID_ARG;

    if(i2c->mutex != NULL)
    {
        vSemaphoreDelete(i2c->mutex);
        i2c->mutex = NULL;
    }
    i2c->i2cHandle = NULL;

    return GSG_SUCCESS;
}

gsg_result_t I2C_IsDeviceReady(void *i2cCtx, uint16_t devAddress, uint32_t trials, uint32_t timeout)
{
    i2c_port_t *i2c = (i2c_port_t *)i2cCtx;

    if(i2c == NULL)
        return GSG_INVALID_ARG;

    if(_i2c_getLock(i2c, timeout))
    {
        if(HAL_I2C_IsDeviceReady(
                i2c->i2cHandle,
                devAddress,
                trials,
                timeout) == HAL_OK)
        {
            _i2c_releaseLock(i2c);
            return GSG_SUCCESS;
        }

        _i2c_releaseLock(i2c);
        return GSG_BUSY;
    }

    return GSG_ERROR;
}
