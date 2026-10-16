
#include <stdint.h>
#include "stm32f1xx_hal.h"
#include "stm32f1xx_hal_can.h"
#include "FreeRTOS.h"
#include "gsg_base.h"
#include "cbus.h"
#include "CANopen.h"
#include "301/CO_ODinterface.h"
#include "OD.h"
#include "services/svar/svar.h"
#include "svar_BSP/svar_bsp.h"

static can_port_t       cbusCANport;
static CO_t *co;

void CO_CANrxDispatch(CO_CANmodule_t *CANmodule, const can_frame_t *frame);
static void cbusTxThread(void *pvParameters);
static void cbusProcessThread(void *pvParameters);
static void cbusRxThread(void *pvParameters);

static TaskHandle_t cbusTxTaskHandle;
static TaskHandle_t cbusRxTaskHandle;
static TaskHandle_t cbusProcessTaskHandle;

volatile static size_t cbusTxStackHighWaterMark;
volatile static size_t cbusRxStackHighWaterMark;
volatile static size_t cbusProcStackHighWaterMark;

void CBUS_Init(void)
{
    // CAN Driver Initialization
    cbusCANport.txQueue = xQueueCreate(CBUS_TX_QUEUE_LENGTH, sizeof(can_frame_t));
    cbusCANport.rxQueue = xQueueCreate(CBUS_RX_QUEUE_LENGTH, sizeof(can_frame_t));
    
    if(CAN_Init(&cbusCANport, &hcan) != GSG_SUCCESS)
    {
        DEBUG_LOGE(DEBUG_TAG_BSP, "CBUS", "CAN Init Failed"); 
    }

    // CANOpen Related Initialization
    CO_ReturnError_t err;
    uint32_t errInfo;

    co = CO_new(NULL, NULL);
    configASSERT(co != NULL);
    err = CO_CANinit(co, &cbusCANport, CBUS_CAN_BITRATE);
    configASSERT(err == CO_ERROR_NO);

    err = CO_CANopenInit( co, NULL, NULL, OD, NULL, 0, CBUS_CAN_BITRATE, CBUS_SDO_TIMEOUT_MS, CBUS_SDO_TIMEOUT_MS, false, CBUS_NODE_ID, &errInfo);
    configASSERT(err == CO_ERROR_NO);
    DEBUG_LOGI(DEBUG_TAG_CAN, "CBUS", "CANopen initialized");

    // Route SDO/PDO access through SVAR (no-op unless CBUS_USE_SVAR_DATABASE)
    CBUS_SVAR_Init();

    // PDO
    err = CO_CANopenInitPDO( co, co->em, OD, 1, &errInfo);
    configASSERT(err == CO_ERROR_NO);

    CO_CANsetNormalMode(co->CANmodule);

    DEBUG_LOGI(DEBUG_TAG_CAN,
           "CBUS",
           "NMT=%d HB=%d",
           co->NMT->operatingState,
           co->NMT->HBproducerTimer);

    // CAN Threads
    xTaskCreate(cbusTxThread, "CBUS_TX", CBUS_TX_TASK_STACK_SIZE, &cbusCANport, CBUS_TX_TASK_PRIORITY, &cbusTxTaskHandle);
    configASSERT(cbusTxTaskHandle != NULL);
    xTaskCreate(cbusRxThread, "CBUS_RX", CBUS_RX_TASK_STACK_SIZE, &cbusCANport, CBUS_RX_TASK_PRIORITY, &cbusRxTaskHandle);
    configASSERT(cbusRxTaskHandle != NULL);
    #ifndef CBUS_DEV_SIM_ENABLED
    xTaskCreate(cbusProcessThread, "CBUS_PROC", CBUS_PROCESS_TASK_STACK_SIZE, NULL, CBUS_PROCESS_TASK_PRIORITY, &cbusProcessTaskHandle);
    configASSERT(cbusProcessTaskHandle != NULL);
    #endif
}

static void cbusProcessThread(void *pvParameters)
{
    uint32_t timerNext_us = 1000;
//    uint8_t readyToRun = 0;

    xTaskNotifyWait(0, 0, NULL, portMAX_DELAY);  // Wait for notification before starting the process thread
    while(1)
    {
        cbusProcStackHighWaterMark = uxTaskGetStackHighWaterMark(NULL);
        
        CO_process(co, false, 1000, &timerNext_us);

        static uint16_t cnt = 0;
        cnt++;
        if(cnt == 1000)
        {
            cnt = 0;
            DEBUG_LOGI(DEBUG_TAG_CAN,
                    "CBUS",
                    "NMT=%d TimerNext=%lu",
                    co->NMT->operatingState,
                    timerNext_us);
        }

        vTaskDelay(pdMS_TO_TICKS(1));
    }
}

static void cbusTxThread(void *pvParameters)
{
    can_port_t *can = (can_port_t *)pvParameters;
    can_frame_t frame;

    while(1)
    {
        /* Wait for a frame to send */
        if(xQueueReceive(can->txQueue, &frame, portMAX_DELAY) == pdTRUE)
        {
            cbusTxStackHighWaterMark = uxTaskGetStackHighWaterMark(NULL);
            //    DEBUG_LOGI(DEBUG_TAG_CAN,
            //        "CAN",
            //        "TX: ID=0x%03X DLC=%d",
            //        frame.id,
            //        frame.dlc);
            // Send the frame
                gsg_result_t result = CAN_SendFrame(can, &frame, 100);
                if(result != GSG_SUCCESS)
                {
                    DEBUG_LOGE(DEBUG_TAG_BSP, "CBUS", "Tx Failed [%d]", result);
                }
        }
        // vTaskDelay(pdMS_TO_TICKS(100));  // Delay for demonstration purposes
    }
} 

static void cbusRxThread(void *pvParameters)
{
    can_port_t *can = (can_port_t *)pvParameters;
    can_frame_t frame;

    while(1)
    {
        cbusRxStackHighWaterMark = uxTaskGetStackHighWaterMark(NULL);
        if(xQueueReceive(can->rxQueue, &frame, portMAX_DELAY) == pdTRUE)
        {
            CO_CANrxDispatch(co->CANmodule, &frame);
        }
    }
}

uint8_t CBUS_sendData(uint32_t nodeAdd, uint8_t *data, uint8_t length)
{
    can_frame_t frame;
    frame.id = nodeAdd;
    frame.idType = CAN_ID_STANDARD;  // or CAN_ID_EXTENDED based on your needs
    frame.frameType = CAN_FRAME_DATA; // or CAN_FRAME_REMOTE if needed
    frame.dlc = length;

    for(uint8_t i = 0; i < length; i++)
    {
        frame.data[i] = data[i];
    }

    // Enqueue the frame for transmission
    if(xQueueSend(cbusCANport.txQueue, &frame, portMAX_DELAY) != pdTRUE)
    {
        DEBUG_LOGE(DEBUG_TAG_BSP, "CAN", "Failed to enqueue CAN Tx");
        return 0;
    }
    return 1;
}

comm_bus_device_t commNetDevice[CBUS_MAX_DEVICES];

uint8_t CBUS_discoverDevice(comm_bus_device_t *device)
{
    
    return 1;    
}

uint8_t CBUS_registerDevice(comm_bus_device_t *device)
{
    return 1;
}

uint8_t CBUS_deRegisterDevice(comm_bus_device_t *device)
{
    return 1;
}

uint8_t CBUS_pingDevice(uint8_t devId)
{
    return 1;
}

uint8_t CBUS_getDeviceInfo(uint8_t devId)
{
    return 1;
}

uint8_t CBUS_receiveData(uint8_t devId, uint16_t regAdd, uint8_t *buffer, uint16_t bufferSize, uint16_t *receivedLength)
{
    return 1;
}

//static uint32_t cbusDummyDeviceData[5][4] =
//{
//    {0x01010089, 0x010200AB, 0x010300CD, 0x010400EF},
//    {0x02010089, 0x020200AB, 0x020300CD, 0x020400EF},
//    {0x03010089, 0x030200AB, 0x030300CD, 0x030400EF},
//    {0x04010089, 0x040200AB, 0x040300CD, 0x040400EF},
//    {0x05010089, 0x050200AB, 0x050300CD, 0x050400EF}
//};
static int32_t cbusDummyDeviceDataSrc[10][10] = 
{
    {0x09, 0x08, 0x07, 0x06, 0x05, 0x04, 0x03, 0x02, 0x01, 0x00},
    {0x09, 0x08, 0x07, 0x06, 0x05, 0x04, 0x03, 0x02, 0x01, 0x00},
    {0x09, 0x08, 0x07, 0x06, 0x05, 0x04, 0x03, 0x02, 0x01, 0x00},
    {0x09, 0x08, 0x07, 0x06, 0x05, 0x04, 0x03, 0x02, 0x01, 0x00},
    {0x09, 0x08, 0x07, 0x06, 0x05, 0x04, 0x03, 0x02, 0x01, 0x00},
    {0x09, 0x08, 0x07, 0x06, 0x05, 0x04, 0x03, 0x02, 0x01, 0x00},
    {0x09, 0x08, 0x07, 0x06, 0x05, 0x04, 0x03, 0x02, 0x01, 0x00},
    {0x09, 0x08, 0x07, 0x06, 0x05, 0x04, 0x03, 0x02, 0x01, 0x00},
    {0x09, 0x08, 0x07, 0x06, 0x05, 0x04, 0x03, 0x02, 0x01, 0x00},
    {0x09, 0x08, 0x07, 0x06, 0x05, 0x04, 0x03, 0x02, 0x01, 0x00},
};
static int32_t cbusDummyDeviceDataSink[10][10] = 
{
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
};

uint8_t CBUS_getChannelData(uint8_t devId, uint8_t channelId, uint8_t *buffer, uint16_t bufferSize)
{
    int32_t value = 0;
    uint8_t i;

    if(devId >= 10 || channelId >= 4)
        return 0;

    value = cbusDummyDeviceDataSrc[devId][channelId];
    for(i = 0; i < bufferSize; i++)
    {
        buffer[i] = (value >> (8 * i)) & 0xFF;
    }
    return 1;
}

uint8_t CBUS_setChannelData(uint8_t devId, uint8_t channelId, uint8_t *buffer, uint16_t bufferSize)
{
    int32_t value = 0;
    uint8_t i;

    if(devId >= 10 || channelId >= 4)
        return 0;

    for(i = 0; i < bufferSize; i++)
    {
        value |= ((int32_t)buffer[i]) << (8 * i);
    }
    cbusDummyDeviceDataSink[devId][channelId] = value;
    
    return 1;
}

// void IO_analogConvCpltCallback(uint8_t channelId, const analog_input_channel_data_t *chData)
// {
//     switch(channelId)
//     {
//         case 0:
//             // SVAR_Set(SVAR_BSP_AN_DATA_CH0, &chData->value);
//             OD_RAM.x4000_peripheralRuntmeData.CH0_DATA = (chData->value * );
//             break;
//         case 1:
//         	// SVAR_Set(SVAR_BSP_AN_DATA_CH1, &chData->value);
//             OD_RAM.x4000_peripheralRuntmeData.CH1_DATA = chData->value;
//             break;
//         case 2:
//             // SVAR_Set(SVAR_BSP_AN_DATA_CH2, &chData->value);
//             OD_RAM.x4000_peripheralRuntmeData.CH2_DATA = chData->value;
//             break;
//         case 3:
//             // SVAR_Set(SVAR_BSP_AN_DATA_CH3, &chData->value);
//             OD_RAM.x4000_peripheralRuntmeData.CH3_DATA = chData->value;
//             break;
//         case 4:
//             // SVAR_Set(SVAR_BSP_AN_DATA_CH4, &chData->value);
//             OD_RAM.x4000_peripheralRuntmeData.CH4_DATA = chData->value;
//             break;
//         case 5:
//             // SVAR_Set(SVAR_BSP_AN_DATA_CH5, &chData->value);
//             OD_RAM.x4000_peripheralRuntmeData.CH5_DATA = chData->value;
//             break;
//         case 6:
//             // SVAR_Set(SVAR_BSP_AN_DATA_CH6, &chData->value);
//             OD_RAM.x4000_peripheralRuntmeData.CH6_DATA = chData->value;
//             break;
//         case 7:
//             // SVAR_Set(SVAR_BSP_AN_DATA_CH7, &chData->value);
//             OD_RAM.x4000_peripheralRuntmeData.CH7_DATA = chData->value;
//             break;
//         default:
//             break;
//     }
// }


