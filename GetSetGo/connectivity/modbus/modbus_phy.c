
/************************************************************************
 * Modbus Physical Layer
 * This file contains the implementation of the Modbus physical layer
 **********************************************************************/

#include "modbus.h"


static modbusTxCallback_t modbusPhyTxCallbacks[_MODBUS_PHY_MAX] = {0};

// Mutex per phy channel to protect access to the callbacks
static osMutexId_t modbusPhyTxMutex[_MODBUS_PHY_MAX];


static uint16_t mbPhyPreTx(modbus_port_t *port, uint8_t *data, uint16_t size);
static uint16_t mbPhyCalculateCRC16(uint8_t *data, uint16_t size);
static uint8_t  mbPhyCalculateLRC(uint8_t *data, uint16_t size);

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

    // Port will be in a valid state to send data, hence no need to validate state
    uint16_t newSize = size;

    newSize = mbPhyPreTx(port, data, size);

    // Call the registered callback for the physical layer
    if (modbusPhyTxCallbacks[port->phy] != NULL)
    {
        osMutexAcquire(modbusPhyTxMutex[port->phy], osWaitForever);
       modbusPhyTxCallbacks[port->phy](data, newSize);

        char hex[20];

        if (DEBUG_LogLevelGet() >= DEBUG_LEVEL_DEBUG)
        {
            sprintf(hex,"Tx[%d]> ",newSize);
            DEBUG_LOGD(DEBUG_TAG_MODBUS,"MB",hex);
            for(uint16_t i=0; i<newSize ; i++ )
            {
                sprintf(hex,"%.2X  ",data[i]); 		DEBUG_LOG_RAW(hex);
            }
        }

        osMutexRelease(modbusPhyTxMutex[port->phy]);
        return GSG_SUCCESS;
    }
    else
    {
        DEBUG_LOGE(DEBUG_TAG_COMM,"MB", "No Tx callback registered for phy");
        return GSG_NOT_IMPLEMENTED; // No callback registered for this physical layer
    }
        
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
        // Calculate and append the CRC16 for the data
        if(port->phy != MODBUS_PHY_TCP && port->phy != MODBUS_PHY_UDP)
        {
            uint16_t crc = mbPhyCalculateCRC16(data, size);
            data[size++] = crc & 0xFF;
            data[size++] = (crc >> 8) & 0xFF;
        }
    }

    if(port->flags.encrypt)
    {
        // Encrypt data if necessary
        // This is a placeholder for actual encryption logic
        // For now, we assume data is already encrypted
    }
    return size; // at the end

}

static uint16_t mbPhyCalculateCRC16(uint8_t *data, uint16_t size)
{
    uint16_t crc = 0xFFFF;
    for (uint16_t i = 0; i < size; i++)
    {
        crc ^= data[i];
        for (uint8_t j = 0; j < 8; j++)
        {
            if (crc & 0x0001)
                crc = (crc >> 1) ^ 0xA001;
            else
                crc >>= 1;
        }
    }
    return crc;
}

static uint8_t mbPhyCalculateLRC(uint8_t *data, uint16_t size)
{
    uint8_t lrc = 0;
    for (uint16_t i = 0; i < size; i++)
    {
        lrc += data[i];
    }
    lrc = (~lrc) + 1; // Two's complement
    return lrc;
}

