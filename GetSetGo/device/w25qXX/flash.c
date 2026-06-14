
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "flash.h"
#include "drv/stm32/SPI/spi.h"

/* ============================================================================
 * Private Helper Functions
 * ========================================================================== */

static bool _flash_wait_ready(flash_device_t *flash, uint32_t timeout)
{
    uint8_t status;
    uint32_t startTick = xTaskGetTickCount();
    
    while (1)
    {
        /* Send RDSR command and read status register */
        uint8_t tx[2] = {W25Q_CMD_READ_SR1, 0xFF};  /* cmd + dummy byte */
        uint8_t rx[2] = {0, 0};
        
        if (SPI_transferData(flash->spiCtx, tx, rx, 2, timeout) != GSG_SUCCESS)
            return false;
        
        status = rx[1];  /* Status is in second byte */
        if ((status & W25Q_SR1_WIP) == 0)
            return true;  /* Device ready */
        
        /* Check timeout */
        if ((xTaskGetTickCount() - startTick) > pdMS_TO_TICKS(timeout))
            return false;
        
        vTaskDelay(pdMS_TO_TICKS(1));
    }
}

static gsg_result_t _flash_write_enable(flash_device_t *flash, uint32_t timeout)
{
    uint8_t cmd = W25Q_CMD_WRITE_ENABLE;
    return SPI_writeData(flash->spiCtx, &cmd, 1, timeout);
}

static gsg_result_t _flash_write_disable(flash_device_t *flash, uint32_t timeout)
{
    uint8_t cmd = W25Q_CMD_WRITE_DISABLE;
    return SPI_writeData(flash->spiCtx, &cmd, 1, timeout);
}

static uint8_t _flash_read_status(flash_device_t *flash, uint32_t timeout)
{
    uint8_t cmd = W25Q_CMD_READ_SR1;
    uint8_t status = 0;
    (void)SPI_transferData(flash->spiCtx, &cmd, &status, 1, timeout);
    return status;
}

/* ============================================================================
 * Public API Functions
 * ========================================================================== */

gsg_result_t FLASH_Init(flash_device_t *flash, void *spiCtx)
{
    if (flash == NULL || spiCtx == NULL)
        return GSG_INVALID_ARG;
    
    flash->spiCtx = spiCtx;
    flash->capacity = W25Q16JV_CAPACITY;
    flash->pageSize = W25Q16JV_PAGE_SIZE;
    
    /* Create mutex for thread-safe access */
    flash->mutex = xSemaphoreCreateMutex();
    if (flash->mutex == NULL)
        return GSG_ERROR;
    
    /* Wait for device to be ready */
    if (!_flash_wait_ready(flash, 1000))
    {
        vSemaphoreDelete(flash->mutex);
        flash->mutex = NULL;
        return GSG_ERROR;
    }
    
    return GSG_SUCCESS;
}

gsg_result_t FLASH_DeInit(flash_device_t *flash)
{
    if (flash == NULL)
        return GSG_INVALID_ARG;
    
    if (flash->mutex != NULL)
    {
        vSemaphoreDelete(flash->mutex);
        flash->mutex = NULL;
    }
    
    flash->spiCtx = NULL;
    return GSG_SUCCESS;
}

gsg_result_t FLASH_readJEDECID(flash_device_t *flash, uint32_t *jedecId)
{
    if (flash == NULL || jedecId == NULL)
        return GSG_INVALID_ARG;
    
    if (xSemaphoreTake(flash->mutex, pdMS_TO_TICKS(1000)) != pdTRUE)
        return GSG_ERROR;
    
    /* JEDEC ID read: send 0x9F command, receive 3 bytes of ID */
    uint8_t tx[4] = {W25Q_CMD_JEDEC_ID, 0xFF, 0xFF, 0xFF};  /* 1 cmd byte + 3 dummy bytes */
    uint8_t rx[4] = {0, 0, 0, 0};
    
    gsg_result_t result = SPI_transferData(flash->spiCtx, tx, rx, 4, 1000);
    
    if (result == GSG_SUCCESS)
    {
        /* ID is in rx[1:3] (first byte is command echo) */
        *jedecId = ((uint32_t)rx[1] << 16) | ((uint32_t)rx[2] << 8) | rx[3];
    }
    
    xSemaphoreGive(flash->mutex);
    return result;
}

gsg_result_t FLASH_readData(flash_device_t *flash, uint8_t *data, uint32_t address, uint32_t size)
{
    if (flash == NULL || data == NULL || (address + size) > flash->capacity)
        return GSG_INVALID_ARG;
    
    if (size == 0)
        return GSG_SUCCESS;
    
    if (xSemaphoreTake(flash->mutex, pdMS_TO_TICKS(5000)) != pdTRUE)
        return GSG_ERROR;
    
    gsg_result_t result = GSG_SUCCESS;
    uint32_t offset = 0;

    if (!_flash_wait_ready(flash, 5000))
    {
        xSemaphoreGive(flash->mutex);
        return GSG_ERROR;
    }
    
    while (offset < size && result == GSG_SUCCESS)
    {
        /* Calculate chunk size - keep it reasonable for stack allocation */
        uint32_t chunk = (size - offset > 128) ? 128 : (size - offset);
        
        /* Prepare tx buffer: [cmd][A23:A16][A15:A8][A7:A0][dummy bytes] */
        uint8_t txBuf[4 + 128];  /* 4 bytes for cmd+addr, up to 128 bytes for data */
        uint8_t rxBuf[4 + 128];
        
        txBuf[0] = W25Q_CMD_READ;
        txBuf[1] = (uint8_t)((address >> 16) & 0xFF);
        txBuf[2] = (uint8_t)((address >> 8) & 0xFF);
        txBuf[3] = (uint8_t)(address & 0xFF);
        
        /* Fill dummy bytes for data read */
        for (uint32_t i = 4; i < (4 + chunk); i++)
        {
            txBuf[i] = 0xFF;  /* Dummy bytes (typically 0xFF for SPI flash) */
        }
        
        /* Full duplex transfer: send cmd+addr+dummy, receive garbage+garbage+garbage+garbage+data */
        if (SPI_transferData(flash->spiCtx, txBuf, rxBuf, 4 + chunk, 5000) != GSG_SUCCESS)
        {
            result = GSG_ERROR;
            break;
        }
        
        /* Extract data from receive buffer (skip first 4 bytes which are cmd echo + addr garbage) */
        memcpy(&data[offset], &rxBuf[4], chunk);
        
        address += chunk;
        offset += chunk;
    }
    
    xSemaphoreGive(flash->mutex);
    return result;
}

gsg_result_t FLASH_writeData(flash_device_t *flash, const uint8_t *data, uint32_t address, uint32_t size)
{
    if (flash == NULL || data == NULL || (address + size) > flash->capacity)
        return GSG_INVALID_ARG;
    
    if (size == 0)
        return GSG_SUCCESS;
    
    if (xSemaphoreTake(flash->mutex, pdMS_TO_TICKS(5000)) != pdTRUE)
        return GSG_ERROR;
    
    gsg_result_t result = GSG_SUCCESS;
    uint32_t offset = 0;
    
    while (offset < size && result == GSG_SUCCESS)
    {
        /* Wait for device to be ready before each page program */
        if (!_flash_wait_ready(flash, 5000))
        {
            result = GSG_ERROR;
            break;
        }
        
        /* Calculate bytes to write in this page */
        uint32_t pageOffset = address & (flash->pageSize - 1);
        uint32_t bytesInPage = flash->pageSize - pageOffset;
        uint32_t toWrite = (size - offset > bytesInPage) ? bytesInPage : (size - offset);
        
        /* Enable write */
        if (_flash_write_enable(flash, 1000) != GSG_SUCCESS)
        {
            result = GSG_ERROR;
            break;
        }
        
        /* Build page program buffer: [0x02][A23:A16][A15:A8][A7:A0][data...] */
        uint8_t combined[4 + 256];
        combined[0] = W25Q_CMD_PAGE_PROGRAM;
        combined[1] = (uint8_t)((address >> 16) & 0xFF);
        combined[2] = (uint8_t)((address >> 8) & 0xFF);
        combined[3] = (uint8_t)(address & 0xFF);
        memcpy(&combined[4], &data[offset], toWrite);
        
        /* Send command + address + data (all in one transfer to keep CS asserted) */
        uint8_t rxBuf[4 + 256];
        if (SPI_transferData(flash->spiCtx, combined, rxBuf, 4 + toWrite, 5000) != GSG_SUCCESS)
        {
            result = GSG_ERROR;
            break;
        }
        
        address += toWrite;
        offset += toWrite;
    }

    if (result == GSG_SUCCESS && !_flash_wait_ready(flash, 5000))
    {
        result = GSG_ERROR;
    }
    
    xSemaphoreGive(flash->mutex);
    return result;
}

gsg_result_t FLASH_eraseSectorSmart(flash_device_t *flash, uint32_t startAddress, uint32_t size)
{
    if (flash == NULL || size == 0)
        return GSG_INVALID_ARG;
    
    /* Validate alignment to smallest sector (4KB) */
    if ((startAddress & (W25Q16JV_SECTOR_4K - 1)) != 0)
        return GSG_INVALID_ARG;  /* Not aligned to 4KB boundary */
    
    /* Validate capacity */
    if ((startAddress + size) > flash->capacity)
        return GSG_INVALID_ARG;
    
    if (xSemaphoreTake(flash->mutex, pdMS_TO_TICKS(60000)) != pdTRUE)
        return GSG_ERROR;
    
    gsg_result_t result = GSG_SUCCESS;
    uint32_t address = startAddress;
    uint32_t remaining = size;
    
    /* Smart sector erase: use largest blocks possible */
    while (remaining > 0 && result == GSG_SUCCESS)
    {
        /* Wait for device to be ready */
        if (!_flash_wait_ready(flash, 5000))
        {
            result = GSG_ERROR;
            break;
        }
        
        /* Enable write */
        if (_flash_write_enable(flash, 1000) != GSG_SUCCESS)
        {
            result = GSG_ERROR;
            break;
        }
        
        uint8_t cmd[4];
        cmd[0] = W25Q_CMD_SECTOR_ERASE_4K;  /* Default to 4KB */
        cmd[1] = (uint8_t)((address >> 16) & 0xFF);
        cmd[2] = (uint8_t)((address >> 8) & 0xFF);
        cmd[3] = (uint8_t)(address & 0xFF);
        uint32_t eraseSize = W25Q16JV_SECTOR_4K;
        
        /* Try to use 64KB block erase if aligned and enough data */
        if ((address & (W25Q16JV_SECTOR_64K - 1)) == 0 && remaining >= W25Q16JV_SECTOR_64K)
        {
            cmd[0] = W25Q_CMD_BLOCK_ERASE_64K;
            eraseSize = W25Q16JV_SECTOR_64K;
        }
        /* Try to use 32KB block erase if aligned and enough data */
        else if ((address & (W25Q16JV_SECTOR_32K - 1)) == 0 && remaining >= W25Q16JV_SECTOR_32K)
        {
            cmd[0] = W25Q_CMD_BLOCK_ERASE_32K;
            eraseSize = W25Q16JV_SECTOR_32K;
        }
        
        /* Send erase command */
        if (SPI_writeData(flash->spiCtx, cmd, 4, 5000) != GSG_SUCCESS)
        {
            result = GSG_ERROR;
            break;
        }
        
        address += eraseSize;
        remaining -= eraseSize;
    }

    if (result == GSG_SUCCESS && !_flash_wait_ready(flash, 60000))
    {
        result = GSG_ERROR;
    }
    
    xSemaphoreGive(flash->mutex);
    return result;
}

gsg_result_t FLASH_eraseSector(flash_device_t *flash, uint32_t startAddress, uint32_t size)
{
    if(flash == NULL || size == 0)
        return GSG_INVALID_ARG;

    if((startAddress & (W25Q16JV_SECTOR_4K - 1)) != 0)
        return GSG_INVALID_ARG;

    if((size & (W25Q16JV_SECTOR_4K - 1)) != 0)
        return GSG_INVALID_ARG;

    if((startAddress + size) > flash->capacity)
        return GSG_INVALID_ARG;

    if(xSemaphoreTake(flash->mutex, pdMS_TO_TICKS(60000)) != pdTRUE)
        return GSG_ERROR;

    gsg_result_t result = GSG_SUCCESS;
    uint32_t address = startAddress;
    uint32_t remaining = size;

    while(remaining > 0)
    {
        if(!_flash_wait_ready(flash, 5000))
        {
            result = GSG_ERROR;
            break;
        }

        if(_flash_write_enable(flash, 1000) != GSG_SUCCESS)
        {
            result = GSG_ERROR;
            break;
        }

        uint8_t cmd[4];

        cmd[0] = W25Q_CMD_SECTOR_ERASE_4K;
        cmd[1] = (uint8_t)((address >> 16) & 0xFF);
        cmd[2] = (uint8_t)((address >> 8) & 0xFF);
        cmd[3] = (uint8_t)(address & 0xFF);

        if(SPI_writeData(flash->spiCtx, cmd, sizeof(cmd), 5000) != GSG_SUCCESS)
        {
            result = GSG_ERROR;
            break;
        }

        address += W25Q16JV_SECTOR_4K;
        remaining -= W25Q16JV_SECTOR_4K;
    }

    if(result == GSG_SUCCESS && !_flash_wait_ready(flash, 60000))
    {
        result = GSG_ERROR;
    }

    xSemaphoreGive(flash->mutex);

    return result;
}

gsg_result_t FLASH_chipErase(flash_device_t *flash)
{
    if (flash == NULL)
        return GSG_INVALID_ARG;
    
    if (xSemaphoreTake(flash->mutex, pdMS_TO_TICKS(120000)) != pdTRUE)
        return GSG_ERROR;
    
    /* Wait for device ready */
    if (!_flash_wait_ready(flash, 5000))
    {
        xSemaphoreGive(flash->mutex);
        return GSG_ERROR;
    }
    
    /* Enable write */
    if (_flash_write_enable(flash, 1000) != GSG_SUCCESS)
    {
        xSemaphoreGive(flash->mutex);
        return GSG_ERROR;
    }
    
    /* Send chip erase command */
    uint8_t cmd = W25Q_CMD_CHIP_ERASE;
    gsg_result_t result = SPI_writeData(flash->spiCtx, &cmd, 1, 5000);
    if (result == GSG_SUCCESS && !_flash_wait_ready(flash, 120000))
    {
        result = GSG_ERROR;
    }
    
    xSemaphoreGive(flash->mutex);
    return result;
}

gsg_result_t FLASH_selfTest(flash_device_t *flash)
{
    if (flash == NULL)
        return GSG_INVALID_ARG;
    
    /* Test parameters */
    #define FLASH_SELFTEST_ADDR     (64U * 1024U)  /* Start at 64KB to avoid boot area */
    #define FLASH_SELFTEST_SIZE     16U
    
    uint8_t testData_write[FLASH_SELFTEST_SIZE];
    uint8_t testData_read[FLASH_SELFTEST_SIZE];
    
    /* Create test pattern */
    for (uint8_t i = 0; i < FLASH_SELFTEST_SIZE; i++)
    {
        testData_write[i] = 0xA5 + i;  /* 0xA5, 0xA6, 0xA7, ... pattern */
    }
    
    /* Erase 4KB sector at test address */
    gsg_result_t result = FLASH_eraseSector(flash, FLASH_SELFTEST_ADDR, W25Q16JV_SECTOR_4K);
    if (result != GSG_SUCCESS)
        return result;
    
    /* Write test data in the middle of the sector (2048 bytes offset) */
    uint32_t writeAddr = FLASH_SELFTEST_ADDR + 2048U;
    result = FLASH_writeData(flash, testData_write, writeAddr, FLASH_SELFTEST_SIZE);
    if (result != GSG_SUCCESS)
        return result;
    
    /* Read back and verify */
    result = FLASH_readData(flash, testData_read, writeAddr, FLASH_SELFTEST_SIZE);
    if (result != GSG_SUCCESS)
        return result;
    
    /* Compare data */
    for (uint8_t i = 0; i < FLASH_SELFTEST_SIZE; i++)
    {
        if (testData_read[i] != testData_write[i])
        {
            return GSG_ERROR;  /* Data mismatch */
        }
    }
    
    return GSG_SUCCESS;
}
