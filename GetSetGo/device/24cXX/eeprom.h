
#ifndef EEPROM_H_
#define EEPROM_H_

#include <stdint.h>
#include "gsg_defs.h"

typedef struct {
    void *context;
    uint32_t size;
    uint8_t devAddress;
    uint8_t __reserved[3];
} eeprom_device_t;

gsg_result_t EE_writeData(void * dev, uint32_t address, uint8_t *data, uint16_t length);
gsg_result_t EE_readData(void * dev, uint32_t address, uint8_t *data, uint16_t length);
gsg_result_t EE_isBusy(void * dev);

#endif /* EEPROM_H_*/ 
