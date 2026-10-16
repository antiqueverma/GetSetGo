#include <stdint.h>
#include <stdlib.h>
#include "cbus.h"

#ifdef CBUS_DEV_SIM_ENABLED

#define CBUS_DEV_SIM_RESPONSE_DELAY_MS 100

// CDS = CBUS Device Simulator
comm_bus_device_t CDS_DCV_SENSE_8CH = {
    .name = "DC Voltage Sensor 8Ch",
    .serialNumber = "OTPS-DCV08-0001",
    .pId = 0,
    .channelCount = 8,
    .swVersion = 1,
    .buffer = NULL,
    .state = CBUS_DEV_STATE_UNREGISTERED
};

cbus_channel_data_t CDS_DCV_SENSE_8CH_CHANNELS[8] = {
    {.value = 100, .precision = 2, .postfix = "V"},
    {.value = 200, .precision = 2, .postfix = "V"},
    {.value = 300, .precision = 2, .postfix = "V"},
    {.value = 400, .precision = 2, .postfix = "V"},
    {.value = 500, .precision = 2, .postfix = "V"},
    {.value = 600, .precision = 2, .postfix = "V"},
    {.value = 700, .precision = 2, .postfix = "V"},
    {.value = 800, .precision = 2, .postfix = "V"},
};

comm_bus_device_t CDS_DCI_SENSE_8CH = {
    .name = "DC Current Sensor 8Ch",
    .serialNumber = "OTPS-DCC08-0002",
    .pId = 0,
    .channelCount = 8,
    .swVersion = 1,
    .buffer = NULL,
    .state = CBUS_DEV_STATE_UNREGISTERED
};

cbus_channel_data_t CDS_DCI_SENSE_8CH_CHANNELS[8] = {
    {.value = 100, .precision = 2, .postfix = "A"},
    {.value = 200, .precision = 2, .postfix = "A"},
    {.value = 300, .precision = 2, .postfix = "A"},
    {.value = 400, .precision = 2, .postfix = "A"},
    {.value = 500, .precision = 2, .postfix = "A"},
    {.value = 600, .precision = 2, .postfix = "A"},
    {.value = 700, .precision = 2, .postfix = "A"},
    {.value = 800, .precision = 2, .postfix = "A"},
};

comm_bus_device_t CDS_RES_SENSE_8CH = {
    .name = "DC Resistance Sensor 8Ch",
    .serialNumber = "OTPS-OHM08-0003",
    .pId = 0,
    .channelCount = 8,
    .swVersion = 1,
    .buffer = NULL,
    .state = CBUS_DEV_STATE_UNREGISTERED
};

cbus_channel_data_t CDS_RES_SENSE_8CH_CHANNELS[8] = {
    {.value = 100, .precision = 2, .postfix = "Ohms"},
    {.value = 200, .precision = 2, .postfix = "Ohms"},
    {.value = 300, .precision = 2, .postfix = "Ohms"},
    {.value = 400, .precision = 2, .postfix = "Ohms"},
    {.value = 500, .precision = 2, .postfix = "Ohms"},
    {.value = 600, .precision = 2, .postfix = "Ohms"},
    {.value = 700, .precision = 2, .postfix = "Ohms"},
    {.value = 800, .precision = 2, .postfix = "Ohms"},
};

comm_bus_device_t CDS_TEMP_SENSE_8CH = {
    .name = "DC Temperature Sensor 8Ch",
    .serialNumber = "OTPS-TMP08-0004",
    .pId = 0,
    .channelCount = 8,
    .swVersion = 1,
    .buffer = NULL,
    .state = CBUS_DEV_STATE_UNREGISTERED
};

cbus_channel_data_t CDS_TEMP_SENSE_8CH_CHANNELS[8] = {
    {.value = 10, .precision = 1, .postfix = "C"},
    {.value = 20, .precision = 1, .postfix = "C"},
    {.value = 30, .precision = 1, .postfix = "C"},
    {.value = 40, .precision = 1, .postfix = "C"},
    {.value = 50, .precision = 1, .postfix = "C"},
    {.value = 60, .precision = 1, .postfix = "C"},
    {.value = 70, .precision = 1, .postfix = "C"},
    {.value = 80, .precision = 1, .postfix = "C"},
};

comm_bus_device_t CDS_PFC_OUTPUT_8CH = {
    .name = "Digital Output 8Ch",
    .serialNumber = "OTPS-DGO08-0005",
    .pId = 0,
    .channelCount = 8,
    .swVersion = 1,
    .buffer = NULL,
    .state = CBUS_DEV_STATE_UNREGISTERED
};

cbus_channel_data_t CDS_PFC_OUTPUT_8CH_CHANNELS[8] = {
    {.value = 0, .precision = 0, .postfix = "b"},
    {.value = 0, .precision = 0, .postfix = "b"},
    {.value = 0, .precision = 0, .postfix = "b"},
    {.value = 0, .precision = 0, .postfix = "b"},
    {.value = 0, .precision = 0, .postfix = "b"},
    {.value = 0, .precision = 0, .postfix = "b"},
    {.value = 0, .precision = 0, .postfix = "b"},
    {.value = 0, .precision = 0, .postfix = "b"},
};

comm_bus_device_t CDS_PFC_INPUT_8CH = {
    .name = "Digital Input 8Ch",
    .serialNumber = "OTPS-DGI08-0007",
    .pId = 0,
    .channelCount = 8,
    .swVersion = 1,
    .buffer = NULL,
    .state = CBUS_DEV_STATE_UNREGISTERED
};

cbus_channel_data_t CDS_PFC_INPUT_8CH_CHANNELS[8] = {
    {.value = 0, .precision = 0, .postfix = "b"},
    {.value = 1, .precision = 0, .postfix = "b"},
    {.value = 0, .precision = 0, .postfix = "b"},
    {.value = 1, .precision = 0, .postfix = "b"},
    {.value = 0, .precision = 0, .postfix = "b"},
    {.value = 1, .precision = 0, .postfix = "b"},
    {.value = 0, .precision = 0, .postfix = "b"},
    {.value = 1, .precision = 0, .postfix = "b"},
};

comm_bus_device_t CDS_PWM_OUTPUT_8CH = {
    .name = "PWM Output 8Ch",
    .serialNumber = "OTPS-PWM08-0006",
    .pId = 0,
    .channelCount = 8,
    .swVersion = 1,
    .buffer = NULL,
    .state = CBUS_DEV_STATE_UNREGISTERED
};

cbus_channel_data_t CDS_PWM_OUTPUT_8CH_CHANNELS[8] = {
    {.value = 10, .precision = 1, .postfix = "%"},
    {.value = 20, .precision = 1, .postfix = "%"},
    {.value = 30, .precision = 1, .postfix = "%"},
    {.value = 40, .precision = 1, .postfix = "%"},
    {.value = 50, .precision = 1, .postfix = "%"},
    {.value = 60, .precision = 1, .postfix = "%"},
    {.value = 70, .precision = 1, .postfix = "%"},
    {.value = 80, .precision = 1, .postfix = "%"},
};

void CBUS_SIM_Init(void)
{
    CDS_DCV_SENSE_8CH.state = CBUS_DEV_STATE_UNREGISTERED;
    CDS_PFC_OUTPUT_8CH.state = CBUS_DEV_STATE_UNREGISTERED;
    CDS_PFC_INPUT_8CH.state = CBUS_DEV_STATE_UNREGISTERED;
    CDS_PWM_OUTPUT_8CH.state = CBUS_DEV_STATE_UNREGISTERED;
}

void CBUS_SIM_devRegister(void)
{
    
}

#endif // CBUS_DEV_SIM_ENABLED
