#include "eeprom.h"
#include "drv/stm32/I2C/i2c.h"

#define EE_TIMEOUT_MS      100

gsg_result_t EE_writeData(void *eeCtx, uint32_t address, uint8_t *data, uint16_t length)
{
    eeprom_device_t *dev = (eeprom_device_t *)eeCtx;

    if(data == NULL || dev == NULL || dev->context == NULL)
        return GSG_INVALID_ARG;

    if((address + length) > dev->size)
        return GSG_INVALID_ARG;

    return I2C_WriteMemory16(
                dev->context,
                dev->devAddress,
                address,
                data,
                length,
                EE_TIMEOUT_MS);
}

gsg_result_t EE_readData(void *eeCtx, uint32_t address, uint8_t *data, uint16_t length)
{
    eeprom_device_t *dev = (eeprom_device_t *)eeCtx;

    if(data == NULL || dev == NULL || dev->context == NULL)
        return GSG_INVALID_ARG;

    if((address + length) > dev->size)
        return GSG_INVALID_ARG;

    return I2C_ReadMemory16(
                dev->context,
                dev->devAddress,
                address,
                data,
                length,
                EE_TIMEOUT_MS);
}

gsg_result_t EE_isBusy(void *eeCtx)
{
    eeprom_device_t *dev = (eeprom_device_t *)eeCtx;

    if(dev == NULL || dev->context == NULL)
        return GSG_INVALID_ARG;

    return I2C_IsDeviceReady(
                dev->context,
                dev->devAddress,
                1,
                EE_TIMEOUT_MS);
}
