#include "can.h"

static can_port_t *canPorts[PORT_PERIPHERAL_CAN_COUNT];

static bool _can_getLock(can_port_t *can, uint32_t timeout)
{
    configASSERT(can != NULL);
    configASSERT(can->mutex != NULL);

    return (xSemaphoreTake(can->mutex, pdMS_TO_TICKS(timeout)) == pdTRUE);
}

static bool _can_releaseLock(can_port_t *can)
{
    configASSERT(can != NULL);
    configASSERT(can->mutex != NULL);

    return (xSemaphoreGive(can->mutex) == pdTRUE);
}

static gsg_result_t _can_halStatusToResult(HAL_StatusTypeDef status)
{
    switch(status)
    {
        case HAL_OK:
            return GSG_SUCCESS;

        case HAL_BUSY:
            return GSG_BUSY;

        case HAL_TIMEOUT:
            return GSG_TIMEOUT;

        case HAL_ERROR:
        default:
            return GSG_ERROR;
    }
}

gsg_result_t CAN_Init(can_port_t *can, CAN_HandleTypeDef *instance)
{
    uint8_t i;
    CAN_FilterTypeDef filter = {0};

    configASSERT(can != NULL);
    configASSERT(instance != NULL);

    can->canHandle = instance;

    can->mutex = xSemaphoreCreateMutex();
    can->txCpltSema = xSemaphoreCreateBinary();

    for(i = 0; i < PORT_PERIPHERAL_CAN_COUNT; i++)
    {
        if(canPorts[i] == NULL)
        {
            canPorts[i] = can;
            break;
        }
    }

    if(i >= PORT_PERIPHERAL_CAN_COUNT || can->mutex == NULL || can->txCpltSema == NULL)
    {
        if(can->mutex != NULL)
            vSemaphoreDelete(can->mutex);

        if(can->txCpltSema != NULL)
            vSemaphoreDelete(can->txCpltSema);

        can->canHandle = NULL;

        return GSG_ERROR;
    }

    filter.FilterBank = 0;
    filter.FilterMode = CAN_FILTERMODE_IDMASK;
    filter.FilterScale = CAN_FILTERSCALE_32BIT;
    filter.FilterIdHigh = 0;
    filter.FilterIdLow = 0;
    filter.FilterMaskIdHigh = 0;
    filter.FilterMaskIdLow = 0;
    filter.FilterFIFOAssignment = CAN_FILTER_FIFO1;
    filter.FilterActivation = ENABLE;
    filter.SlaveStartFilterBank = 14;

    if(HAL_CAN_ConfigFilter(can->canHandle, &filter) != HAL_OK)
        return GSG_ERROR;

    if(HAL_CAN_Start(can->canHandle) != HAL_OK)
    {
        DEBUG_LOGE(DEBUG_TAG_CAN,
               "CAN",
               "HAL_CAN_Start failed: err=0x%08lX state=%d MSR=0x%08lX (check RX/PA11 idle level, transceiver)",
               HAL_CAN_GetError(can->canHandle),
               (int)HAL_CAN_GetState(can->canHandle),
               (unsigned long)can->canHandle->Instance->MSR);
        return GSG_ERROR;
    }
    
    DEBUG_LOGI(DEBUG_TAG_CAN,
           "CAN",
           "Started. Free Mailboxes=%lu",
           HAL_CAN_GetTxMailboxesFreeLevel(can->canHandle));

    if(HAL_CAN_ActivateNotification(can->canHandle,
            CAN_IT_RX_FIFO1_MSG_PENDING |
            CAN_IT_RX_FIFO1_OVERRUN |
            // CAN_IT_TX_MAILBOX_EMPTY |
            CAN_IT_ERROR |
            CAN_IT_BUSOFF) != HAL_OK)
    {
        return GSG_ERROR;
    }

    return GSG_SUCCESS;
}

gsg_result_t CAN_SendFrame(can_port_t *can, const can_frame_t *frame, uint32_t timeout)
{
    CAN_TxHeaderTypeDef txHeader = {0};
    uint32_t txMailbox;

    configASSERT(can != NULL);
    configASSERT(can->canHandle != NULL);
    configASSERT(frame != NULL);

    if(!_can_getLock(can, timeout))
        return GSG_ERROR;

    txHeader.IDE = (frame->idType == CAN_ID_STANDARD) ? CAN_ID_STD : CAN_ID_EXT;
    txHeader.RTR = (frame->frameType == CAN_FRAME_DATA) ? CAN_RTR_DATA : CAN_RTR_REMOTE;
    txHeader.StdId = frame->id;
    txHeader.ExtId = frame->id;
    txHeader.DLC = frame->dlc;
    txHeader.TransmitGlobalTime = DISABLE;

    HAL_StatusTypeDef status = HAL_CAN_AddTxMessage(can->canHandle,
                                                    &txHeader,
                                                    (uint8_t *)frame->data,
                                                    &txMailbox);

    if(status != HAL_OK)
    {
        DEBUG_LOGE(DEBUG_TAG_CAN,
               "CAN",
               "AddTx failed: status=%d err=0x%08lX free=%lu",
               status,
               HAL_CAN_GetError(can->canHandle),
               HAL_CAN_GetTxMailboxesFreeLevel(can->canHandle));
        _can_releaseLock(can);
        return _can_halStatusToResult(status);
    }

    // Disabling for now because we are using blocking send for simplicity. In a real application, you might want to handle this asynchronously.
    // if(xSemaphoreTake(can->txCpltSema, pdMS_TO_TICKS(timeout)) != pdTRUE)
    // {
    //     _can_releaseLock(can);
    //     return GSG_TIMEOUT;
    // }

    _can_releaseLock(can);

    return GSG_SUCCESS;
}
void HAL_CAN_RxFifo1MsgPendingCallback(CAN_HandleTypeDef *hcan)
{
    BaseType_t higherPriorityTaskWoken = pdFALSE;
    CAN_RxHeaderTypeDef rxHeader;
    can_frame_t frame;
    uint8_t i;

    for(i = 0; i < PORT_PERIPHERAL_CAN_COUNT; i++)
    {
        if(canPorts[i] != NULL && canPorts[i]->canHandle == hcan)
            break;
    }

    if(i >= PORT_PERIPHERAL_CAN_COUNT || canPorts[i]->rxQueue == NULL)
        return;

    while(HAL_CAN_GetRxFifoFillLevel(hcan, CAN_RX_FIFO1) > 0U)
    {
        if(HAL_CAN_GetRxMessage(hcan, CAN_RX_FIFO1, &rxHeader, frame.data) != HAL_OK)
            break;

        if(rxHeader.IDE == CAN_ID_STD)
        {
            frame.id = rxHeader.StdId;
            frame.idType = CAN_ID_STANDARD;
        }
        else
        {
            frame.id = rxHeader.ExtId;
            frame.idType = CAN_ID_EXTENDED;
        }

        frame.frameType = (rxHeader.RTR == CAN_RTR_DATA) ? CAN_FRAME_DATA : CAN_FRAME_REMOTE;
        frame.dlc = (uint8_t)rxHeader.DLC;

        // Drop the frame if the queue is full; the ISR must never block
        xQueueSendFromISR(canPorts[i]->rxQueue, &frame, &higherPriorityTaskWoken);
    }

    portYIELD_FROM_ISR(higherPriorityTaskWoken);
}
