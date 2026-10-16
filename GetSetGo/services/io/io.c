#include "FreeRTOS.h"
#include "task.h"
#include "gsg_defs.h"
#include "gsg_config.h"
#include "gsg_mcu.h"
#include "drv/stm32/GPIO/gpio.h"
#include "io.h"

#ifndef GSG_IO_ADC_VREF_PLUS_uV 
    #error "GSG_IO_ADC_VREF_PLUS_uV not defined in gsg_config.h"
#elif (GSG_IO_ADC_VREF_PLUS_uV <= 0)
    #error "GSG_IO_ADC_VREF_PLUS_uV must be greater than 0"
#endif

#ifndef PORT_PERIPHERAL_ADC_RESOLUTION
    #error "PORT_PERIPHERAL_ADC_RESOLUTION not defined in port.h"
#elif ((PORT_PERIPHERAL_ADC_RESOLUTION <= 0) || (PORT_PERIPHERAL_ADC_RESOLUTION > 32))
    #error "PORT_PERIPHERAL_ADC_RESOLUTION must be a valid resolution value greater than 0"
#else
    #define IO_ADC_MAX_COUNT            ((UINT64_C(1) << PORT_PERIPHERAL_ADC_RESOLUTION) - UINT64_C(1))
    #define IO_ADC_ROUNDING_BIAS        (IO_ADC_MAX_COUNT / 2)
#endif

#ifndef GSG_IO_ANALOG_CONTINUOUS_CONVERSION
    #error "GSG_IO_ANALOG_CONTINUOUS_CONVERSION not defined in gsg_config.h"
#endif

#define IO_CAL_ROUNDING_BIAS                500
#define GSG_IO_PRECISION_ROUNDING_BIAS      500000

#if defined(GSG_TRACK_MEMORY_STATS) && (GSG_TRACK_MEMORY_STATS == GSG_ENABLE)
volatile static size_t ioTaskHighWaterMark = 0;
#endif

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
    configASSERT(ioTaskHandle != NULL);
    return GSG_SUCCESS;
}

// to be called from ADC ISR or DMA callback to update the analog channel data
static gsg_result_t IO_WriteAnalogChannel(uint8_t channelId, adc_data_t rawData)
{
    if(channelId >= GSG_IO_MAX_ANALOG_CHANNELS)
        return GSG_INVALID_ARG;

    if(analogChannels[channelId] == NULL)
        return GSG_NOT_FOUND;

    analogChannels[channelId]->data.rawCount    = rawData;
    analogChannels[channelId]->data.minCount = (analogChannels[channelId]->data.minCount < rawData) ? analogChannels[channelId]->data.minCount : rawData;
    analogChannels[channelId]->data.maxCount = (analogChannels[channelId]->data.maxCount > rawData) ? analogChannels[channelId]->data.maxCount : rawData;
    return GSG_SUCCESS;
}

static void ioTaskHandler(void *args)
{
    uint8_t i = 0;
    TickType_t lastWakeTime = xTaskGetTickCount();

    for(;;)
    {
        #if defined(GSG_TRACK_MEMORY_STATS) && (GSG_TRACK_MEMORY_STATS == GSG_ENABLE)
        ioTaskHighWaterMark = uxTaskGetStackHighWaterMark(ioTaskHandle);
        #endif

        #if (GSG_IO_ANALOG_CONTINUOUS_CONVERSION)
        // Process analog data channels continuously
        for(i = 0; i < GSG_IO_MAX_ANALOG_CHANNELS; i++)
        {
            if(analogChannels[i] != NULL && analogChannels[i]->flags.enable)
            {
                ioProcessAnalogChannel(analogChannels[i]);
            }
        }
        #else 
        #error "IO Module One-shot conversion mode is currently under development"
        // Process analog data channels on demand
        uint32_t channelToConvert = 0xFF;
        xTaskNotifyWait(0, ULONG_MAX, &channelToConvert, portMAX_DELAY);
        if((channelToConvert < GSG_IO_MAX_ANALOG_CHANNELS) && (analogChannels[channelToConvert] != NULL) && (analogChannels[channelToConvert]->flags.enable))
        {
            // Handle the notification value if needed
            ioProcessAnalogChannel(analogChannels[channelToConvert]);
        }
        #endif

        // Process digital input channels
        for(uint8_t i = 0; i < GSG_IO_MAX_DIGITAL_CHANNELS; i++)
        {
            if(digitalChannels[i] != NULL && digitalChannels[i]->flags.enable)
            {
                ioProcessDigitalChannel(digitalChannels[i]);
            }
        }

        vTaskDelayUntil(&lastWakeTime, pdMS_TO_TICKS(GSG_IO_CONTINUOUS_MODE_PERIOD_MS));
    }
}

static void ioProcessAnalogChannel(analog_input_channel_t *channel)
{
    int64_t calibratedValue;
    gsg_result_t result;

    channel->flags.busy = 1;
    result = ADC_ReadChannel(channel->channelId, &channel->data.rawCount);
    if(result != GSG_SUCCESS)
    {
        channel->flags.error = 1;
        channel->flags.busy = 0;
        return;
    }
    
    channel->flags.error = 0;

    if(channel->flags.firstSample)
    {
        channel->data.filteredCount = channel->data.rawCount;

        // Convert ADC count to voltage in µV
        channel->data.adcVoltage_uV = (analog_data_t)(((int64_t)channel->data.filteredCount * GSG_IO_ADC_VREF_PLUS_uV + IO_ADC_ROUNDING_BIAS) / IO_ADC_MAX_COUNT);

        calibratedValue = ((int64_t)channel->data.adcVoltage_uV) * channel->calGain_x1000;
        if(calibratedValue < 0)
            calibratedValue = (calibratedValue - IO_CAL_ROUNDING_BIAS) / 1000; // Rounding off
        else
            calibratedValue = (calibratedValue + IO_CAL_ROUNDING_BIAS) / 1000; // Rounding off
        calibratedValue += channel->calOffset;

        // Apply output precision
        int64_t scaledValue = calibratedValue * channel->precision;
        if(scaledValue < 0)
            scaledValue = (scaledValue - GSG_IO_PRECISION_ROUNDING_BIAS) / 1000000; // Rounding off
        else
            scaledValue = (scaledValue + GSG_IO_PRECISION_ROUNDING_BIAS) / 1000000; // Rounding off
        channel->data.value = (analog_data_t)scaledValue; // Copy the scaled value to the channel data

        channel->data.minCount = channel->data.rawCount;
        channel->data.maxCount = channel->data.rawCount;
        channel->data.minValue = channel->data.value;
        channel->data.maxValue = channel->data.value;

        channel->flags.firstSample = 0;
    }
    else
    {
        // Apply Exponential Moving Average (EMA) filter to the raw ADC count
        // EMA y[n] = α * x[n] + (1 - α) * y[n-1]
        channel->data.filteredCount += (analog_data_t)(((int64_t)((int64_t)channel->data.rawCount - channel->data.filteredCount) * channel->emaAlpha_x1000) / 1000);

        // Convert ADC count to voltage in µV
        channel->data.adcVoltage_uV = (analog_data_t)(((int64_t)channel->data.filteredCount * GSG_IO_ADC_VREF_PLUS_uV + IO_ADC_ROUNDING_BIAS) / IO_ADC_MAX_COUNT);

        // Apply calibration
        calibratedValue = ((int64_t)channel->data.adcVoltage_uV) * channel->calGain_x1000; 
        if(calibratedValue < 0)
            calibratedValue = (calibratedValue - IO_CAL_ROUNDING_BIAS) / 1000; // Rounding off
        else
            calibratedValue = (calibratedValue + IO_CAL_ROUNDING_BIAS) / 1000; // Rounding off
        calibratedValue += channel->calOffset;

        // Apply output precision
        int64_t scaledValue = calibratedValue * channel->precision;
        if(scaledValue < 0)
            scaledValue = (scaledValue - GSG_IO_PRECISION_ROUNDING_BIAS) / 1000000; // Rounding off
        else
            scaledValue = (scaledValue + GSG_IO_PRECISION_ROUNDING_BIAS) / 1000000; // Rounding off
        channel->data.value = (analog_data_t)scaledValue; // Rounding off

        if(channel->data.rawCount < channel->data.minCount)
            channel->data.minCount = channel->data.rawCount;

        if(channel->data.rawCount > channel->data.maxCount)
            channel->data.maxCount = channel->data.rawCount;

        if(channel->data.value < channel->data.minValue)
            channel->data.minValue = channel->data.value;

        if(channel->data.value > channel->data.maxValue)
            channel->data.maxValue = channel->data.value;
    }
    channel->flags.busy = 0;
    IO_analogConvCpltCallback(channel->channelId, &channel->data);
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

    channel->debounceElapsedMs += GSG_IO_CONTINUOUS_MODE_PERIOD_MS;

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
        analogChannels[channelId]->emaAlpha_x1000         = GSG_IO_DEFAULT_EMA_ALPHA;      // Default α = 0.1
        
        analogChannels[channelId]->precision        = precision;  
        analogChannels[channelId]->calGain_x1000    = gain;     
        analogChannels[channelId]->calOffset        = offset;   

        analogChannels[channelId]->data.rawCount            = 0;
        analogChannels[channelId]->data.maxCount         = 0;
        analogChannels[channelId]->data.minCount         = 0;
        analogChannels[channelId]->data.filteredCount    = 0;

        analogChannels[channelId]->data.value            = 0;
        analogChannels[channelId]->data.minValue         = 0;
        analogChannels[channelId]->data.maxValue         = 0;

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

    analogChannels[channelId]->emaAlpha_x1000 = alpha;

    return GSG_SUCCESS;
}

gsg_result_t IO_GetAnalogValue(uint8_t channelId, analog_data_t *value)
{
    #if (GSG_IO_ANALOG_CONTINUOUS_CONVERSION)
    if((channelId >= GSG_IO_MAX_ANALOG_CHANNELS) || (value == NULL))
        return GSG_INVALID_ARG;

    if(analogChannels[channelId] == NULL)
        return GSG_NOT_FOUND;

    if(analogChannels[channelId]->flags.error)
        return GSG_ERROR;

    *value = analogChannels[channelId]->data.value;
    return GSG_SUCCESS;
    #else
    Notify the Io Task to perform a one-shot conversion for the specified channel
    xTaskNotify(ioTaskHandle, (uint32_t)channelId, eNoAction);
    // Wait for conversion complete notification
    xTaskNotifyWait(0, 0, NULL, portMAX_DELAY);
    return GSG_SUCCESS;
    #endif
}

gsg_result_t IO_GetAnalogMinMaxValues(uint8_t channelId, analog_data_t *minValue, analog_data_t *maxValue)
{
    if((channelId >= GSG_IO_MAX_ANALOG_CHANNELS) || (minValue == NULL) || (maxValue == NULL))
        return GSG_INVALID_ARG;

    if(analogChannels[channelId] == NULL)
        return GSG_NOT_FOUND;

    if(analogChannels[channelId]->flags.error)
        return GSG_ERROR;

    *minValue = analogChannels[channelId]->data.minValue;
    *maxValue = analogChannels[channelId]->data.maxValue;
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

    *count = analogChannels[channelId]->data.rawCount;
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

    *minCount = analogChannels[channelId]->data.minCount;
    *maxCount = analogChannels[channelId]->data.maxCount;
    return GSG_SUCCESS;
}

gsg_result_t IO_setAnalogCalibration(uint8_t channelId, int32_t gain_x1000, analog_data_t offset)
{
    if(channelId >= GSG_IO_MAX_ANALOG_CHANNELS)
        return GSG_INVALID_ARG;

    if(analogChannels[channelId] == NULL)
        return GSG_NOT_FOUND;
    
    analogChannels[channelId]->calGain_x1000 = gain_x1000;
    analogChannels[channelId]->calOffset = offset;

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

__weak void IO_analogConvCpltCallback(uint8_t channelId, const analog_input_channel_data_t *chData)
{
    (void)channelId;
    (void)chData;
}

