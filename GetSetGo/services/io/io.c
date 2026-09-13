#include "FreeRTOS.h"
#include "task.h"
#include "gsg_defs.h"
#include "port/stm32f407ve/port.h"
#include "drv/stm32/GPIO/gpio.h"
#include "io.h"


static TaskHandle_t ioTaskHandle;
analog_input_channel_t *analogChannels[GSG_IO_MAX_ANALOG_CHANNELS];
digital_input_channel_t *digitalChannels[GSG_IO_MAX_DIGITAL_CHANNELS];

static void ioTaskHandler(void *args);
static void ioProcessAnalogChannel(analog_input_channel_t *channel);
static void ioProcessDigitalChannel(digital_input_channel_t *channel);

gsg_result_t IO_Init(void)
{
    for(uint8_t i = 0; i < GSG_IO_MAX_ANALOG_CHANNELS; i++)
    {
        analogChannels[i] = NULL;
    }

    for(uint8_t i = 0; i < GSG_IO_MAX_DIGITAL_CHANNELS; i++)
    {
        digitalChannels[i] = NULL;
    }

    xTaskCreate(ioTaskHandler, "IO", GSG_IO_TASK_STACK_SIZE, NULL, GSG_IO_TASK_PRIORITY, &ioTaskHandle);
    return GSG_SUCCESS;
}

// to be called from ADC ISR or DMA callback to update the analog channel data
static gsg_result_t IO_WriteAnalogChannel(uint8_t channelId, adc_data_t rawData)
{
    if(channelId >= GSG_IO_MAX_ANALOG_CHANNELS)
        return GSG_INVALID_ARG;

    if(analogChannels[channelId] == NULL)
        return GSG_NOT_FOUND;

    analogChannels[channelId]->count    = rawData;
    analogChannels[channelId]->minCount = (analogChannels[channelId]->minCount < rawData) ? analogChannels[channelId]->minCount : rawData;
    analogChannels[channelId]->maxCount = (analogChannels[channelId]->maxCount > rawData) ? analogChannels[channelId]->maxCount : rawData;
    return GSG_SUCCESS;
}

static void ioTaskHandler(void *args)
{
    uint8_t i = 0;
    TickType_t lastWakeTime = xTaskGetTickCount();

    for(;;)
    {
        // Process analog data channels
        for(i = 0; i < GSG_IO_MAX_ANALOG_CHANNELS; i++)
        {
            if(analogChannels[i] != NULL && analogChannels[i]->flags.enable)
            {
                ioProcessAnalogChannel(analogChannels[i]);
            }
        }

        // Process digital input channels
        for(uint8_t i = 0; i < GSG_IO_MAX_DIGITAL_CHANNELS; i++)
        {
            if(digitalChannels[i] != NULL && digitalChannels[i]->flags.enable)
            {
                ioProcessDigitalChannel(digitalChannels[i]);
            }
        }

        vTaskDelayUntil(&lastWakeTime, pdMS_TO_TICKS(GSG_IO_TASK_PERIOD_MS));
    }
}

static void ioProcessAnalogChannel(analog_input_channel_t *channel)
{
    analog_data_t calibratedValue;
    gsg_result_t result;

    result = ADC_ReadChannel(channel->channelId, &channel->count);
    if(result != GSG_SUCCESS)
    {
        channel->flags.error = 1;
        channel->flags.busy = 0;
        return;
    }

    channel->flags.busy = 1;

    if(channel->flags.firstSample)
    {
        channel->filteredCount = channel->count;

        // Convert ADC count to voltage in µV
        channel->convertedValue = (analog_data_t)(((int64_t)channel->filteredCount * 3300000 + 2047) / 4095);

        calibratedValue = (analog_data_t)(((int64_t)channel->convertedValue * channel->calGain + 500) / 1000);
        calibratedValue += channel->calOffset;

        // Apply output precision
        channel->value = (analog_data_t)(((int64_t)calibratedValue * channel->precision + 500000) / 1000000);

        channel->minCount = channel->count;
        channel->maxCount = channel->count;
        channel->minValue = channel->value;
        channel->maxValue = channel->value;

        channel->flags.firstSample = 0;
    }
    else
    {
        // Apply Exponential Moving Average (EMA) filter to the raw ADC count
        // EMA y[n] = α * x[n] + (1 - α) * y[n-1]
        channel->filteredCount += (analog_data_t)(((int64_t)((int64_t)channel->count - channel->filteredCount) * channel->emaAlpha) / 1000);

        // Convert ADC count to voltage in µV
        channel->convertedValue = (analog_data_t)(((int64_t)channel->filteredCount * 3300000 + 2047) / 4095);

        // Apply calibration
        calibratedValue = (analog_data_t)(((int64_t)channel->convertedValue * channel->calGain + 500) / 1000); // Rounding off
        calibratedValue += channel->calOffset;

        // Apply output precision
        channel->value = (analog_data_t)(((int64_t)calibratedValue * channel->precision + 500000) / 1000000);

        if(channel->count < channel->minCount)
            channel->minCount = channel->count;

        if(channel->count > channel->maxCount)
            channel->maxCount = channel->count;

        if(channel->value < channel->minValue)
            channel->minValue = channel->value;

        if(channel->value > channel->maxValue)
            channel->maxValue = channel->value;
    }

    channel->flags.busy = 0;
}

static void ioProcessDigitalChannel(digital_input_channel_t *channel)
{
    gpio_pin_state_t rawState;
    gsg_result_t result;

    result = GPIO_readPin(channel->gpioPin, &rawState);

    if(result != GSG_SUCCESS)
    {
        channel->flags.error = 1;
        return;
    }

    channel->flags.error = 0;
    channel->rawState = rawState;

    if(channel->rawState == channel->state)
    {
        channel->debounceElapsedMs = 0;
        return;
    }

    channel->debounceElapsedMs += GSG_IO_TASK_PERIOD_MS;

    if(channel->debounceElapsedMs >= channel->debounceTimeMs)
    {
        channel->state = channel->rawState;
        channel->debounceElapsedMs = 0;
    }
}

/* Public API for Analog Interface */ 

gsg_result_t IO_ConfigureAnalogChannel(uint8_t channelId, uint16_t gain, analog_data_t offset, uint16_t precision)
{
    if((channelId >= GSG_IO_MAX_ANALOG_CHANNELS) || (precision == 0) || (precision > 10000) || (gain == 0))
        return GSG_INVALID_ARG;

    // Fill the designated slot for the analog channel
    if(analogChannels[channelId] == NULL)
    {
        analogChannels[channelId] = (analog_input_channel_t *)pvPortMalloc(sizeof(analog_input_channel_t));
        if(analogChannels[channelId] == NULL)
            return GSG_OVERFLOW;

        analogChannels[channelId]->channelId = channelId;
        analogChannels[channelId]->emaAlpha         = GSG_IO_DEFAULT_EMA_ALPHA;      // Default α = 0.1
        
        analogChannels[channelId]->precision        = precision;  
        analogChannels[channelId]->calGain          = gain;     
        analogChannels[channelId]->calOffset        = offset;   

        analogChannels[channelId]->count            = 0;
        analogChannels[channelId]->maxCount         = 0;
        analogChannels[channelId]->minCount         = 0;
        analogChannels[channelId]->filteredCount    = 0;

        analogChannels[channelId]->value            = 0;
        analogChannels[channelId]->minValue         = 0;
        analogChannels[channelId]->maxValue         = 0;

        analogChannels[channelId]->flags.busy       = 0;
        analogChannels[channelId]->flags.error      = 0;
        analogChannels[channelId]->flags.firstSample= 1;
        analogChannels[channelId]->flags.enable     = 1;
    }
    
    return GSG_SUCCESS;
}

gsg_result_t IO_ConfigureAnalogEMA(uint8_t channelId, uint16_t alpha)
{
    if((channelId >= GSG_IO_MAX_ANALOG_CHANNELS) || (alpha == 0) || (alpha > 1000))
        return GSG_INVALID_ARG;

    if(analogChannels[channelId] == NULL)
        return GSG_NOT_FOUND;

    analogChannels[channelId]->emaAlpha = alpha;

    return GSG_SUCCESS;
}

gsg_result_t IO_GetAnalogValue(uint8_t channelId, analog_data_t *value)
{
    if((channelId >= GSG_IO_MAX_ANALOG_CHANNELS) || (value == NULL))
        return GSG_INVALID_ARG;

    if(analogChannels[channelId] == NULL)
        return GSG_NOT_FOUND;

    if(analogChannels[channelId]->flags.error)
        return GSG_ERROR;

    *value = analogChannels[channelId]->value;
    return GSG_SUCCESS;
}

gsg_result_t IO_GetAnalogMinMaxValues(uint8_t channelId, analog_data_t *minValue, analog_data_t *maxValue)
{
    if((channelId >= GSG_IO_MAX_ANALOG_CHANNELS) || (minValue == NULL) || (maxValue == NULL))
        return GSG_INVALID_ARG;

    if(analogChannels[channelId] == NULL)
        return GSG_NOT_FOUND;

    if(analogChannels[channelId]->flags.error)
        return GSG_ERROR;

    *minValue = analogChannels[channelId]->minValue;
    *maxValue = analogChannels[channelId]->maxValue;
    return GSG_SUCCESS;
}

gsg_result_t IO_GetAnalogCount(uint8_t channelId, adc_data_t *count)
{
    if((channelId >= GSG_IO_MAX_ANALOG_CHANNELS) || (count == NULL))
        return GSG_INVALID_ARG;

    if(analogChannels[channelId] == NULL)
        return GSG_NOT_FOUND;

    if(analogChannels[channelId]->flags.error)
        return GSG_ERROR;

    *count = analogChannels[channelId]->count;
    return GSG_SUCCESS;
}

gsg_result_t IO_GetAnalogMinMaxCounts(uint8_t channelId, adc_data_t *minCount, adc_data_t *maxCount)
{
    if((channelId >= GSG_IO_MAX_ANALOG_CHANNELS) || (minCount == NULL) || (maxCount == NULL))
        return GSG_INVALID_ARG;

    if(analogChannels[channelId] == NULL)
        return GSG_NOT_FOUND;

    if(analogChannels[channelId]->flags.error)
        return GSG_ERROR;

    *minCount = analogChannels[channelId]->minCount;
    *maxCount = analogChannels[channelId]->maxCount;
    return GSG_SUCCESS;
}

/* Public API for Digital Interface */ 

gsg_result_t IO_GetDigitalStateByPin(gpio_pin_t inputPin, gpio_pin_state_t *state)
{
    if(state == NULL)
        return GSG_INVALID_ARG;

    for(uint8_t i = 0; i < GSG_IO_MAX_DIGITAL_CHANNELS; i++)
    {
        if(digitalChannels[i] != NULL && digitalChannels[i]->gpioPin == inputPin)
        {
            *state = GPIO_PIN_RESET;
            if(digitalChannels[i]->flags.error)
                return GSG_ERROR;

            *state = digitalChannels[i]->state;
            return GSG_SUCCESS;
        }
    }

    return GSG_NOT_FOUND;
}

gsg_result_t IO_GetDigitalStateByIndex(uint8_t channelIndex, gpio_pin_state_t *state)
{
    if(state == NULL)
        return GSG_INVALID_ARG;

    if(channelIndex >= GSG_IO_MAX_DIGITAL_CHANNELS)
        return GSG_INVALID_ARG;

    if(digitalChannels[channelIndex] == NULL)
        return GSG_NOT_FOUND;

    *state = GPIO_PIN_RESET;
    if(digitalChannels[channelIndex]->flags.error)
        return GSG_ERROR;

    *state = digitalChannels[channelIndex]->state;
    return GSG_SUCCESS;
}

gsg_result_t IO_GetDigitalIndexByPin(gpio_pin_t inputPin, uint8_t *idx)
{
    if((idx == NULL) || (inputPin >= __GPIO_PIN_COUNT))
        return GSG_INVALID_ARG;

    for(uint8_t i = 0; i < GSG_IO_MAX_DIGITAL_CHANNELS; i++)
    {
        if(digitalChannels[i] != NULL && digitalChannels[i]->gpioPin == inputPin)
        {
            *idx = i;
            return GSG_SUCCESS;
        }
    }

    return GSG_NOT_FOUND;
}

gsg_result_t IO_ConfigureDigitalChannel(gpio_pin_t inputPin, uint16_t debounceTimeMs)
{
    uint8_t i = 0;
    // Check if the channel is already configured, then update the pin property
    for(i = 0; i < GSG_IO_MAX_DIGITAL_CHANNELS; i++)
    {
        if(digitalChannels[i] != NULL && digitalChannels[i]->gpioPin == inputPin)
        {
            digitalChannels[i]->debounceTimeMs = debounceTimeMs;
            digitalChannels[i]->debounceElapsedMs = 0;
            return GSG_SUCCESS;
        }
    }

    // If the channel is not configured, find an empty slot to configure it
    for(i = 0; i < GSG_IO_MAX_DIGITAL_CHANNELS; i++)
    {
        if(digitalChannels[i] == NULL)
        {
            digitalChannels[i] = (digital_input_channel_t *)pvPortMalloc(sizeof(digital_input_channel_t));
            if(digitalChannels[i] == NULL)
                return GSG_OVERFLOW;
            
            digitalChannels[i]->gpioPin = inputPin;
            digitalChannels[i]->debounceTimeMs = debounceTimeMs;
            digitalChannels[i]->debounceElapsedMs = 0;
            digitalChannels[i]->rawState = GPIO_PIN_RESET;
            digitalChannels[i]->state = GPIO_PIN_RESET;
            digitalChannels[i]->flags.busy = 0;
            digitalChannels[i]->flags.error = 0;
            digitalChannels[i]->flags.enable = 1;
            return GSG_SUCCESS;
        }
    }
    return GSG_OVERFLOW;
}



