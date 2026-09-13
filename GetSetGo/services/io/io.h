
#ifndef IO_H_
#define IO_H_

#include <stdint.h>
#include <stdbool.h>
#include "drv/stm32/adc/adc.h"
#include "drv/stm32/GPIO/gpio.h"

#define GSG_IO_TASK_STACK_SIZE  256
#define GSG_IO_TASK_PRIORITY    7
#define GSG_IO_TASK_PERIOD_MS   10

#define GSG_IO_MAX_DIGITAL_CHANNELS 10
#define GSG_IO_MAX_ANALOG_CHANNELS  PORT_PERIPHERAL_ADC_CHANNEL_COUNT

#define GSG_IO_DEFAULT_EMA_ALPHA    100
#define GSG_IO_DEFAULT_CAL_GAIN     1000
#define GSG_IO_DEFAULT_PRECISION    100
#define GSG_IO_DEFAULT_CAL_OFFSET   0

#define ADC_MAX_COUNT       4095U
#define ADC_VDDA_UV         3300000L

typedef int32_t analog_data_t; 

typedef struct {
    uint8_t busy:1;
    uint8_t error:1;
    uint8_t enable:1;
    uint8_t firstSample:1;
    uint8_t __reserved:4;
} io_ch_flags_t;

typedef struct {
    adc_data_t      count;
    adc_data_t      maxCount;
    adc_data_t      minCount;
    adc_data_t      filteredCount;
    
    analog_data_t   convertedValue; // ADC voltage in µV
    analog_data_t   value;
    analog_data_t   maxValue;
    analog_data_t   minValue;
    analog_data_t   calOffset;

    uint16_t        precision;  // Output scaling, max = 10000
    uint16_t        calGain;    // Gain × 1000
    uint16_t        emaAlpha;   // Alpha × 1000
    uint8_t         channelId;
    io_ch_flags_t   flags;
} analog_input_channel_t;

typedef struct {
    uint16_t            debounceTimeMs;
    uint16_t            debounceElapsedMs;
    gpio_pin_t          gpioPin;
    gpio_pin_state_t    rawState;
    gpio_pin_state_t    state;
    io_ch_flags_t       flags;
} digital_input_channel_t;

gsg_result_t IO_ConfigureAnalogChannel(uint8_t channelId, uint16_t gain, analog_data_t offset, uint16_t precision);
gsg_result_t IO_ConfigureAnalogEMA(uint8_t channelId, uint16_t alpha);
gsg_result_t IO_GetAnalogValue(uint8_t channelId, analog_data_t *value);
gsg_result_t IO_GetAnalogMinMaxValues(uint8_t channelId, analog_data_t *minValue, analog_data_t *maxValue);
gsg_result_t IO_GetAnalogCount(uint8_t channelId, adc_data_t *count);
gsg_result_t IO_GetAnalogMinMaxCounts(uint8_t channelId, adc_data_t *minCount, adc_data_t *maxCount);
gsg_result_t IO_ConfigureDigitalChannel(gpio_pin_t inputPin, uint16_t debounceTimeMs);
gsg_result_t IO_GetDigitalStateByPin(gpio_pin_t inputPin, gpio_pin_state_t *state);
gsg_result_t IO_GetDigitalIndexByPin(gpio_pin_t inputPin, uint8_t *idx);
gsg_result_t IO_Init(void);

#endif /* IO_H_ */
