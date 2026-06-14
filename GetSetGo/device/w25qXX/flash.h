
#ifndef FLASH_H_
#define FLASH_H_

#include <stdint.h>
#include <stdbool.h>
#include "FreeRTOS.h"
#include "semphr.h"
#include "gsg_defs.h"

/* w25q16jv Device Constants */
#define W25Q16JV_JEDEC_ID           0xEF4015    /* Expected JEDEC ID for w25q16jv */
#define W25Q16JV_CAPACITY           (2U * 1024U * 1024U)  /* 2MB total */
#define W25Q16JV_PAGE_SIZE          256U        /* Minimum write unit */
#define W25Q16JV_SECTOR_4K          (4U * 1024U)  /* 4KB sector (smallest erase) */
#define W25Q16JV_SECTOR_32K         (32U * 1024U) /* 32KB block erase */
#define W25Q16JV_SECTOR_64K         (64U * 1024U) /* 64KB block erase */

/* w25q16jv SPI Commands */
#define W25Q_CMD_JEDEC_ID           0x9F    /* Read JEDEC ID */
#define W25Q_CMD_READ               0x03    /* Read Data */
#define W25Q_CMD_FREAD              0x0B    /* Fast Read (requires dummy byte) */
#define W25Q_CMD_PAGE_PROGRAM       0x02    /* Page Program (PP) */
#define W25Q_CMD_SECTOR_ERASE_4K    0x20    /* Sector Erase (4KB) */
#define W25Q_CMD_BLOCK_ERASE_32K    0x52    /* 32KB Block Erase */
#define W25Q_CMD_BLOCK_ERASE_64K    0xD8    /* 64KB Block Erase */
#define W25Q_CMD_CHIP_ERASE         0xC7    /* Chip Erase */
#define W25Q_CMD_WRITE_ENABLE       0x06    /* Write Enable (WREN) */
#define W25Q_CMD_WRITE_DISABLE      0x04    /* Write Disable (WRDI) */
#define W25Q_CMD_READ_SR1           0x05    /* Read Status Register 1 */
#define W25Q_CMD_WRITE_SR1          0x01    /* Write Status Register 1 */
#define W25Q_CMD_RELEASE_POWERDOWN  0xAB    /* Release from Deep Power-Down */
#define W25Q_CMD_DEEP_POWERDOWN     0xB9    /* Deep Power-Down */

/* Status Register 1 Bits */
#define W25Q_SR1_WIP                0x01    /* Write In Progress (bit 0) */
#define W25Q_SR1_WEL                0x02    /* Write Enable Latch (bit 1) */
#define W25Q_SR1_BP_MASK            0x3C    /* Block Protect bits (bits 2-5) */
#define W25Q_SR1_SEC                0x40    /* Sector Protect (bit 6) */
#define W25Q_SR1_SRP                0x80    /* Status Register Protect (bit 7) */

/* littlefs compatibility notes:
 * These drivers are designed for littlefs lower layer:
 * - FLASH_readData():  Implements read() callback (arbitrary size)
 * - FLASH_writeData(): Implements prog() callback (no auto-erase, caller erases)
 * - FLASH_eraseSector(): Implements erase() callback with smart sector selection
 * 
 * littlefs configuration example:
 * - read_size: 256 (W25Q16JV page size)
 * - prog_size: 256 (W25Q16JV page size)
 * - block_size: 4096 (W25Q16JV smallest sector - 4KB)
 * - block_count: 512 (2MB / 4KB)
 * - cache_size: 256
 */

typedef struct {
    void *spiCtx;               /* SPI context for HAL access */
    SemaphoreHandle_t mutex;    /* Thread-safe access */
    uint32_t capacity;          /* Device capacity in bytes */
    uint16_t pageSize;          /* Page size (write unit) */
    uint8_t __reserved[2];
} flash_device_t;

/* Public API */
gsg_result_t FLASH_Init(flash_device_t *flash, void *spiCtx);
gsg_result_t FLASH_DeInit(flash_device_t *flash);
gsg_result_t FLASH_readData(flash_device_t *flash, uint8_t *data, uint32_t address, uint32_t size);
gsg_result_t FLASH_writeData(flash_device_t *flash, const uint8_t *data, uint32_t address, uint32_t size);
gsg_result_t FLASH_eraseSector(flash_device_t *flash, uint32_t startAddress, uint32_t size);
gsg_result_t FLASH_chipErase(flash_device_t *flash);
gsg_result_t FLASH_readJEDECID(flash_device_t *flash, uint32_t *jedecId);
gsg_result_t FLASH_selfTest(flash_device_t *flash);

#endif /* FLASH_H_ */
