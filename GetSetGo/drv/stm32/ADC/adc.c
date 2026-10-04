#include "gsg_mcu.h"

#if defined(HAL_ADC_MODULE_ENABLED)
#include "gsg_defs.h"
#include "adc.h"

extern ADC_HandleTypeDef hadc1;
extern ADC_HandleTypeDef hadc2;
extern ADC_HandleTypeDef hadc3;

#define ADC_LOGICAL_CHANNEL_COUNT  PORT_PERIPHERAL_ADC_CHANNEL_COUNT 

#if defined(GSG_MCU_STM32F1)
#define ADC_SAMPLE_TIME  ADC_SAMPLETIME_13CYCLES_5
#else
#define ADC_SAMPLE_TIME  ADC_SAMPLETIME_15CYCLES
#endif

typedef struct
{
    ADC_HandleTypeDef *instance;
    uint32_t physicalChannel;
} adc_channel_map_t;

static adc_port_t adcPort;

static const adc_channel_map_t adcChannelMap[ADC_LOGICAL_CHANNEL_COUNT] =
{
    {&hadc1, ADC_CHANNEL_0},
    {&hadc1, ADC_CHANNEL_1},
    {&hadc1, ADC_CHANNEL_2},
    {&hadc1, ADC_CHANNEL_3},
    // {&hadc1, ADC_CHANNEL_4},
    // {&hadc1, ADC_CHANNEL_5},
    // {&hadc1, ADC_CHANNEL_6},
    // {&hadc1, ADC_CHANNEL_7},
    // {&hadc1, ADC_CHANNEL_8},
    // {&hadc1, ADC_CHANNEL_9},
    // {&hadc1, ADC_CHANNEL_10},
    // {&hadc1, ADC_CHANNEL_11},
    // {&hadc1, ADC_CHANNEL_12},
    // {&hadc1, ADC_CHANNEL_13},
    // {&hadc1, ADC_CHANNEL_14},
    // {&hadc1, ADC_CHANNEL_15},
    // {&hadc1, ADC_CHANNEL_TEMPSENSOR},
    {&hadc1, ADC_CHANNEL_VREFINT}
};

gsg_result_t ADC_Init(adc_port_t *adc)
{
    if(adc == NULL)
        return GSG_INVALID_ARG;

    adc->channelCount = ADC_LOGICAL_CHANNEL_COUNT;
    adc->resolution = 12;

    adcPort = *adc;

    return GSG_SUCCESS;
}

gsg_result_t ADC_ReadChannel(uint8_t channel, adc_data_t *count)
{
    ADC_HandleTypeDef *hadc;
    ADC_ChannelConfTypeDef config = {0};

    if(count == NULL)
        return GSG_INVALID_ARG;

    if(channel >= adcPort.channelCount)
        return GSG_INVALID_ARG;

    hadc = adcChannelMap[channel].instance;

    config.Channel = adcChannelMap[channel].physicalChannel;
    config.Rank = 1;
    config.SamplingTime = ADC_SAMPLE_TIME;

    if(HAL_ADC_ConfigChannel(hadc, &config) != HAL_OK)
        return GSG_ERROR;

    if(HAL_ADC_Start(hadc) != HAL_OK)
        return GSG_ERROR;

    if(HAL_ADC_PollForConversion(hadc, 100) != HAL_OK)
    {
        HAL_ADC_Stop(hadc);
        return GSG_ERROR;
    }

    *count = HAL_ADC_GetValue(hadc);

    if(HAL_ADC_Stop(hadc) != HAL_OK)
        return GSG_ERROR;

    return GSG_SUCCESS;
}

#endif /* HAL_ADC_MODULE_ENABLED */
