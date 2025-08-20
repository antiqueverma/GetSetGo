/*
 * NVM.h
 *
 *  Created on: May 1, 2023
 *      Author: antiq
 */

#ifndef DRIVERS_NVM_H_
#define DRIVERS_NVM_H_

#include	"GetSetGoConfig.h"

#define NVM_USE_BUFFER_POOL			0//1

#define NVM_WORD_SIZE		1		//keep this 1byte unless needed
#define NVM_CELL_RST_VALUE	0xFF	//keep the same

#ifndef NVM_SIZE
#warning "NVM_SIZE not defined. Default NVM size will be 1KB"
#define NVM_SIZE			1024
#endif

#ifndef NVM_WORD_SIZE
#warning "NVM_WORD_SIZE not defined. Default word size will be 1 Byte"
#define NVM_WORD_SIZE		1
#endif

#ifndef NVM_CELL_RST_VALUE
	#if	NVM_WORD_SIZE == 2
		#warning "NVM_CELL_RST_VALUE not defined. Default value will be 0xFFFF"
		#define NVM_CELL_RST_VALUE		0xFFFF
	#else
		#warning "NVM_CELL_RST_VALUE not defined. Default value will be 0xFF"
		#define NVM_CELL_RST_VALUE		0xFF
	#endif
#endif

#if	NVM_WORD_SIZE == 1
	#define nvm_data_t	uint8_t
#elif NVM_WORD_SIZE == 2
	#define nvm_data_t	uint16_t
#endif

#define nvm_address_t	uint32_t
#define NVM_Max_Valid_Address	(NVM_SIZE/NVM_WORD_SIZE)


typedef enum {
	NVM_READ,
	NVM_WRITE

}nvm_rqst_type_t;

typedef struct {
    TaskHandle_t    task;       // Handle of the requesting task (4 bytes)

    uint8_t         *dataPtr;   // Pointer to the data buffer (4 bytes)

    nvm_address_t    address;    // Address in the NVM (4 bytes)

    uint16_t        length;     // Length of data to read/write (2 bytes)
    nvm_rqst_type_t type;      // Enum for request type (1 bytes)
    uint8_t			padding;
} nvm_rqst_t;

void NVM_Init(void);

#if (GSG_USE_RTOS == 0)  //bare metal
	bool NVM_Write_Cell(uint16_t Address, NVM_Word_Type Byte);
	bool NVM_Read_Cell(nvm_address_t Address, nvm_data_t *ptr)
#elif (GSG_USE_RTOS == 1)	//freeRTOS
#define NVM_RQST_QUEUE_LENGTH		60
	/**
	 * @brief Request data from or write data to the NVM task.
	 *
	 * This function sends a request to the NVM task for either reading or writing data.
	 * It populates an nvm_rqst_t structure with the required parameters and places it
	 * into the request queue. The requesting task is expected to handle synchronization
	 * via task notifications or other mechanisms.
	 *
	 * @param[in] task      TaskHandle of the requesting task (for notifications).
	 * @param[in] address   Address in the NVM for read/write operations.
	 * @param[in] destPtr   Pointer to the data buffer for storing read data or
	 *                      containing data to be written. Can be NULL if using
	 *                      the static buffer pool in the NVM module.
	 * @param[in] length    Number of bytes to read/write.
	 * @param[in] type      Type of request (e.g., NVM_READ or NVM_WRITE).
	 *
	 * @return pdPASS if the request was successfully placed in the queue.
	 * @return pdFAIL if the queue was full or the request could not be placed.
	 *
	 * @note The `length` parameter should not exceed the maximum size supported
	 *       by the NVM module. Ensure `destPtr` is valid if not using the buffer pool.
	 *
	 * @attention The requesting task must ensure synchronization for retrieving
	 *            the data or verifying the write operation's success.
	 * @attention The requesting task must ensure that the pointer passed (if not NULL)
	 *            does not go out of scope until the NVM task completes the operation.
	 */
	BaseType_t NVM_rqstData(TaskHandle_t task, nvm_address_t address, void *destPtr, uint16_t length, nvm_rqst_type_t type);
	TaskHandle_t nvmTaskHandle;
#endif

//unsigned char NVM_Write_String(unsigned int Base_Address, NVM_Word_Type *String, unsigned char Length);
//BaseType_t NVM_rqstDataRead(uint16_t address, TaskHandle_t task);
//BaseType_t NVM_rqstDataWrite(uint16_t address, uint8_t data, TaskHandle_t task);

#endif /* DRIVERS_NVM_H_ */
