/*
 * NVM.h
 *
 *  Created on: May 1, 2023
 *      Author: antiq
 */

#include	"gsg_config.h"

#if (defined (GSG_USE_NVM) && (GSG_USE_NVM == GSG_ENABLE))
#ifndef NVM_H_
#define NVM_H_

#include <stdint.h>
#include <stdbool.h>
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "timers.h"
#include "gsg_defs.h"

#define NVM_USE_BUFFER_POOL			0//1
#define NVM_CELL_RST_VALUE			0xFF	//keep the same
// #ifndef NVM_SIZE
// 	#warning "NVM_SIZE not defined. Default NVM size will be 1KB"
// 	#define NVM_SIZE					1024
// #endif

#ifndef NVM_WORD_SIZE
	#define NVM_WORD_SIZE				1
#endif

#ifndef NVM_CELL_RST_VALUE
	#if	NVM_WORD_SIZE == 4
		#define NVM_CELL_RST_VALUE		0xFFFFFFFF
	#elif	NVM_WORD_SIZE == 2
		#define NVM_CELL_RST_VALUE		0xFFFF
	#else
		#define NVM_CELL_RST_VALUE		0xFF
	#endif
#endif

#ifndef NVM_READ_TIMEOUT_MS 
	#define NVM_READ_TIMEOUT_MS 1000
#endif

#ifndef NVM_WRITE_TIMEOUT_MS 
	#define NVM_WRITE_TIMEOUT_MS 1000
#endif

#define NVM_ENABLE_PERIODIC_PAGE_WRITE	0
#ifndef NVM_PAGE_WRITE_INTERVAL_S
	#define NVM_PAGE_WRITE_INTERVAL_MS 	60000
#else
	#define NVM_PAGE_WRITE_INTERVAL_MS 	(NVM_PAGE_WRITE_INTERVAL_S*1000)
#endif

#define NVM_READ_DELAY_MS  0
#define NVM_WRITE_DELAY_MS 6

#if	NVM_WORD_SIZE == 1
	typedef uint8_t nvm_data_t;
#elif NVM_WORD_SIZE == 2
	typedef uint16_t nvm_data_t;
#endif
typedef uint32_t nvm_address_t;	

typedef gsg_result_t (*nvm_write_fn_t)(void *context, nvm_address_t address, nvm_data_t *data, uint16_t length);
typedef gsg_result_t (*nvm_read_fn_t)(void *context, nvm_address_t address, nvm_data_t *data, uint16_t length);
typedef gsg_result_t (*nvm_pageErase_fn_t)(void *context, nvm_address_t address, nvm_data_t *data, uint16_t length);
typedef gsg_result_t (*nvm_busy_poll_fn_t)(void *context);

typedef struct {
	void *context;
	const uint32_t 	size;
	const uint32_t 	startAddress;
	const uint32_t 	endAddress;


	uint8_t 	*dummyMemPtr;
	const nvm_write_fn_t writeFn;
	const nvm_read_fn_t readFn;
	const nvm_pageErase_fn_t pageEraseFn;
	const nvm_busy_poll_fn_t isBusyFn;

	#if (GSG_OS_USED == GSG_OS_FREERTOS)	//freeRTOS
	SemaphoreHandle_t nvmMutex;
	TimerHandle_t nvmWriteTimerHandle;
	#endif

	const uint16_t  	pageSize;
	uint16_t  			postWriteDelayMs;
	uint16_t  			postReadDelayMs;
	uint16_t  			busyTimeoutMs;

	uint8_t 			busy:2;
	uint8_t 			mounted:1;
	uint8_t 			isReadonly:1;
	const uint8_t 		handlePageRollOver:1;
	const uint8_t 		dummy:1;
	uint8_t 			__reserved:2;

} nvm_device_t;

gsg_result_t NVM_Init(nvm_device_t *device);
gsg_result_t NVM_DeInit(nvm_device_t *device);
gsg_result_t NVM_writeData(nvm_device_t *device, nvm_address_t address, nvm_data_t *data, uint16_t length);
gsg_result_t NVM_readData(nvm_device_t *device, nvm_address_t address, nvm_data_t *data, uint16_t length);

#endif /* NVM_H_ */

#endif
