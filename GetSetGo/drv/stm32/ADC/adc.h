

#ifndef ADC_H_
#define ADC_H_

#include <stdint.h>

typedef uint32_t adc_data_t; 

typedef struct {
    uint8_t channelCount;
    uint8_t resolution; // 12, 10, 8, 6 bits
} adc_port_t;

gsg_result_t ADC_Init(adc_port_t *adc);
gsg_result_t ADC_ReadChannel(uint8_t channel, adc_data_t *count);

#endif /* ADC_H_ */