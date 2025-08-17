
/************************************************************************
 * Modbus Physical Layer
 * This file contains the implementation of the Modbus physical layer
 **********************************************************************/

#include "modbus.h"


static modbusTxCallback_t modbusPhyTxCallbacks[_MODBUS_PHY_MAX] = {0};
static mbPhyRxCbContext_t *mbPhyRxCbContext[_MODBUS_PHY_MAX] = {NULL};

// Mutex per phy channel to protect access to the callbacks
static osMutexId_t modbusPhyTxMutex[_MODBUS_PHY_MAX];


static uint16_t mbPhyPreTx(modbus_port_t *port, uint8_t *data, uint16_t size);
//static uint8_t  mbPhyCalculateLRC(uint8_t *data, uint16_t size);

gsg_result_t MB_registerPhyTxCallback(modbus_phy_t phy, modbusTxCallback_t cb)
{
    if (phy >= _MODBUS_PHY_MAX || cb == NULL)
        return GSG_INVALID_ARG;

    // Register the callback for the specified physical layer
    modbusPhyTxCallbacks[phy] = cb;
    
    // Initialize the mutex for this phy channel
    modbusPhyTxMutex[phy] = osMutexNew(NULL);

    if (modbusPhyTxMutex[phy] == NULL)
        return GSG_ERROR; // Failed to create mutex

    return GSG_SUCCESS;
}

gsg_result_t MB_unregisterPhyTxCallback(modbus_phy_t phy)
{
    if (phy >= _MODBUS_PHY_MAX)
        return GSG_INVALID_ARG;

    // Unregister the callback for the specified physical layer
    modbusPhyTxCallbacks[phy] = NULL;
    // Delete the mutex for this phy channel
    if (modbusPhyTxMutex[phy] != NULL)
    {
        osMutexDelete(modbusPhyTxMutex[phy]);
        modbusPhyTxMutex[phy] = NULL;
    }
    return GSG_SUCCESS;
}

gsg_result_t mbPhySendData(modbus_port_t *port, uint8_t *data, uint16_t size)
{
    if (port == NULL || data == NULL || size == 0)
        return GSG_INVALID_ARG;
        
    uint8_t phy = MODBUS_PHY_NONE;
    if (port->slave[data[0]] != NULL)
        phy = port->slave[data[0]]->phy;
    if(modbusPhyTxCallbacks[phy] == NULL)
    {
//        DEBUG_LOGI(DEBUG_TAG_COMM,"MB", "Slave-TxCb == NULL, using port's default");
        phy = port->phy;
    }

    // Port will be in a valid state to send data, hence no need to validate state
//    uint16_t newSize = size;
//    newSize = mbPhyPreTx(port, data, size);

    // Call the registered callback for the physical layer
    osMutexAcquire(modbusPhyTxMutex[phy], osWaitForever);
    modbusPhyTxCallbacks[phy](data, size);
    osMutexRelease(modbusPhyTxMutex[phy]);

   char hex[20];
   if (DEBUG_LogLevelGet() >= DEBUG_LEVEL_DEBUG)
   {
       sprintf(hex,"Tx[%d]> ",size);
       DEBUG_LOGD(DEBUG_TAG_MODBUS,"MB",hex);
       for(uint16_t i=0; i<size ; i++ )

       {
           sprintf(hex,"%.2X  ",data[i]); 		DEBUG_LOG_RAW(hex);
       }
   }
    return GSG_SUCCESS;
}

static uint16_t mbPhyPreTx(modbus_port_t *port, uint8_t *data, uint16_t size)
{
    // port struct is already validated in mbMaster_sendData
//    uint16_t newSize = 0;
    if(port->flags.asciiMode)
    {
        // Convert data to ASCII format if necessary
        // This is a placeholder for actual conversion logic
        // For now, we assume data is already in ASCII format
    }
    else
    {
        // // Calculate and append the CRC16 for the data
        // if(port->phy != MODBUS_PHY_TCP && port->phy != MODBUS_PHY_UDP)
        // {
        //     uint16_t crc = mbPhyCalculateCRC16(data, size);
        //     data[size++] = crc & 0xFF;
        //     data[size++] = (crc >> 8) & 0xFF;
        // }
    }

    if(port->flags.encrypt)
    {
        ;
    }
    return size; // at the end

}

gsg_result_t mbPhyPreRx(modbus_port_t *port, uint8_t *data, uint16_t size)
{
    if (port == NULL || data == NULL || size == 0)
        return GSG_INVALID_ARG;

    if(port->flags.encrypt)
    {
        ;
    }

    return GSG_SUCCESS;
}

//static uint8_t mbPhyCalculateLRC(uint8_t *data, uint16_t size)
//{
//    uint8_t lrc = 0;
//    for (uint16_t i = 0; i < size; i++)
//    {
//        lrc += data[i];
//    }
//    lrc = (~lrc) + 1; // Two's complement
//    return lrc;
//}

// Rx Callback for ISRs
void MB_phyRxByteISRCallback(modbus_phy_t phy, uint8_t byte)
{
    mbPhyRxCbContext_t *ctx = mbPhyRxCbContext[phy];

    // Check if context is valid
    if (ctx == NULL)
        return;

    // Check if queue is valid
    if (ctx->rxQueueHandle == NULL)
        return;

//    ctx->lastRxByteTime = osKernelGetTickCount();

    // Directly push the byte data into queue
    if(osMessageQueuePut(ctx->rxQueueHandle, &byte, 0, 0) != osOK)
    {
        DEBUG_LOGE(DEBUG_TAG_MODBUS, "MB", "Rx buffer overflow");
    }
}

gsg_result_t MB_registerPhyRxContext(modbus_phy_t phy, modbus_port_t *port, mbPhyRxCbContext_t *context)
{
    if (phy >= _MODBUS_PHY_MAX || context == NULL || port == NULL)
        return GSG_INVALID_ARG;

    // Register the Rx context for the specified physical layer
    mbPhyRxCbContext[phy] = context;

    // Initialize the Rx queue for this phy channel
    context->rxQueueHandle = osMessageQueueNew(MODBUS_PORT_RX_BUFFER_SIZE, sizeof(uint8_t), NULL);
    if (context->rxQueueHandle == NULL)
        return GSG_ERROR; // Failed to create message queue

    context->lastRxByteTime = 0;

    // Initialize the Rx buffer and counters
    // memset(context->rxBuffer, 0, sizeof(context->rxBuffer));
    // context->byteCtr = 0;

    // Associate the Rx context with the Modbus port
    port->rxCtx[phy] = context;

    return GSG_SUCCESS;
}

gsg_result_t MB_unregisterPhyRxContext(modbus_phy_t phy, modbus_port_t *port)
{
    if (phy >= _MODBUS_PHY_MAX || port == NULL)
        return GSG_INVALID_ARG;

    // Unregister the Rx context for the specified physical layer
    mbPhyRxCbContext[phy] = NULL;

    // Delete the Rx queue for this phy channel
    if (port->rxCtx[phy] != NULL && port->rxCtx[phy]->rxQueueHandle != NULL)
    {
        osMessageQueueDelete(port->rxCtx[phy]->rxQueueHandle);
        port->rxCtx[phy]->rxQueueHandle = NULL;
    }

    // Reset the Rx context in the Modbus port
    port->rxCtx[phy] = NULL;

    return GSG_SUCCESS;
}
