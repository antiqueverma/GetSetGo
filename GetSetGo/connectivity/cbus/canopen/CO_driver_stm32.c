
#include <string.h>
#include "301/CO_driver.h"
#include "drv/stm32/CAN/can.h"

CO_ReturnError_t CO_CANsend(CO_CANmodule_t *CANmodule, CO_CANtx_t *buffer)
{
    can_frame_t frame;

    (void)CANmodule;

    if(buffer == NULL)
    {
        return CO_ERROR_ILLEGAL_ARGUMENT;
    }

    // DEBUG_LOGI(DEBUG_TAG_CAN,
    //        "CO",
    //        "CO_CANsend() ID=0x%03X",
    //        buffer->ident);

    can_port_t *can = (can_port_t *)CANmodule->CANptr;

    frame.id        = buffer->ident;
    frame.idType    = CAN_ID_STANDARD;
    frame.frameType = CAN_FRAME_DATA;
    frame.dlc       = buffer->DLC;

    memcpy(frame.data, buffer->data, frame.dlc);

    if(xQueueSend(can->txQueue, &frame, portMAX_DELAY) != pdTRUE)
    {
        return CO_ERROR_TX_OVERFLOW;
    }
    buffer->bufferFull = false;
    return CO_ERROR_NO;
}

CO_ReturnError_t CO_CANmodule_init(CO_CANmodule_t *CANmodule, void *CANptr, CO_CANrx_t rxArray[], uint16_t rxSize, CO_CANtx_t txArray[], uint16_t txSize, uint16_t CANbitRate)
{
    if(CANmodule == NULL || rxArray == NULL || txArray == NULL)
    {
        return CO_ERROR_ILLEGAL_ARGUMENT;
    }

    CANmodule->CANptr = CANptr;

    CANmodule->rxArray = rxArray;
    CANmodule->rxSize = rxSize;

    CANmodule->txArray = txArray;
    CANmodule->txSize = txSize;

    CANmodule->CANerrorStatus = 0;
    CANmodule->CANnormal = true;
    CANmodule->useCANrxFilters = false;
    CANmodule->bufferInhibitFlag = false;
    CANmodule->firstCANtxMessage = true;
    CANmodule->CANtxCount = 0;
    CANmodule->errOld = 0;

    memset(rxArray, 0, sizeof(CO_CANrx_t) * rxSize);
    memset(txArray, 0, sizeof(CO_CANtx_t) * txSize);

    (void)CANbitRate;

    return CO_ERROR_NO;
}

CO_CANtx_t *CO_CANtxBufferInit(CO_CANmodule_t *CANmodule, uint16_t index, uint16_t ident, bool_t rtr, uint8_t noOfBytes, bool_t syncFlag)
{
    CO_CANtx_t *buffer;

    if(CANmodule == NULL || index >= CANmodule->txSize)
    {
        return NULL;
    }

    buffer = &CANmodule->txArray[index];

    buffer->ident = ident;
    buffer->DLC = noOfBytes;
    buffer->bufferFull = false;
    buffer->syncFlag = syncFlag;

    (void)rtr;

    return buffer;
}

CO_ReturnError_t CO_CANrxBufferInit(CO_CANmodule_t *CANmodule, uint16_t index, uint16_t ident, uint16_t mask, bool_t rtr, void *object, void (*CANrx_callback)(void *object, void *message))
{
    CO_CANrx_t *buffer;

    if(CANmodule == NULL)
        return CO_ERROR_ILLEGAL_ARGUMENT;

    if(index >= CANmodule->rxSize)
        return CO_ERROR_ILLEGAL_ARGUMENT;

    buffer = &CANmodule->rxArray[index];

    buffer->ident = ident;
    buffer->mask = mask;
    buffer->object = object;
    buffer->CANrx_callback = CANrx_callback;

    (void)rtr;

    return CO_ERROR_NO;
}

void CO_CANsetConfigurationMode(void *CANptr)
{
    (void)CANptr;
}

void CO_CANsetNormalMode(CO_CANmodule_t *CANmodule)
{
    CANmodule->CANnormal = true;
}

void CO_CANmodule_disable(CO_CANmodule_t *CANmodule)
{
    (void)CANmodule;
}

void CO_CANmodule_process(CO_CANmodule_t *CANmodule)
{
    (void)CANmodule;
}

void CO_CANrxDispatch(CO_CANmodule_t *CANmodule, const can_frame_t *frame)
{
    CO_CANrxMsg_t msg;
    uint16_t i;

    if(CANmodule == NULL || frame == NULL ||
       frame->idType != CAN_ID_STANDARD || frame->frameType != CAN_FRAME_DATA)
    {
        return;
    }

    msg.ident = (uint16_t)frame->id;
    msg.DLC = (frame->dlc > 8U) ? 8U : frame->dlc;
    memcpy(msg.data, frame->data, msg.DLC);

    for(i = 0; i < CANmodule->rxSize; i++)
    {
        CO_CANrx_t *rx = &CANmodule->rxArray[i];

        if(rx->CANrx_callback != NULL && ((msg.ident ^ rx->ident) & rx->mask) == 0U)
        {
            rx->CANrx_callback(rx->object, &msg);
            break;
        }
    }
}