/*
 * NVM.c
 *
 *  Created on: May 1, 2023
 *      Author: antiq
 */

#include "nvm.h"
#if (defined(GSG_USE_NVM) && (GSG_USE_NVM == GSG_ENABLE))
typedef enum {
	NVM_READ,
	NVM_WRITE
}nvm_acc_type_t;

#ifdef NVM_USE_DUMMY_NVM
	static nvm_data_t NVM_DUMMY_MEMORY[NVM_SIZE];
#endif

#if (GSG_OS_USED == GSG_OS_BARE_METAL)	//bare metal
	static volatile uint8_t nvmBusyFlag = 0;
#endif

static void nvmPageWriteTimerCallback(TimerHandle_t xTimer);

static bool getNvmLock(nvm_device_t *dev, nvm_acc_type_t accType)
{
	if(dev == NULL)
		return false;

	#if (GSG_OS_USED == GSG_OS_FREERTOS)	//freeRTOS
	if(dev->nvmMutex == NULL)
		return false;
	if(accType == NVM_WRITE)
		return ((bool)xSemaphoreTake(dev->nvmMutex, pdMS_TO_TICKS(NVM_WRITE_TIMEOUT_MS)));
	else
		return ((bool)xSemaphoreTake(dev->nvmMutex, pdMS_TO_TICKS(NVM_READ_TIMEOUT_MS)));
	#elif (GSG_OS_USED == GSG_OS_BARE_METAL)	//bare metal
	return ((dev->busy == 0)?true:false);
	#endif
}

static void releaseNvmLock(nvm_device_t *dev)
{
	if(dev == NULL)
		return;

	#if (GSG_OS_USED == GSG_OS_FREERTOS)	//freeRTOS
	if(dev->nvmMutex != NULL)
		xSemaphoreGive(dev->nvmMutex);
	#elif (GSG_OS_USED == GSG_OS_BARE_METAL)	//bare metal
	nvmBusyFlag = 0;
	#endif
}

static uint8_t accessNvmCell(nvm_device_t *dev, nvm_address_t address, nvm_data_t *data, uint16_t length, nvm_acc_type_t accType)
{
	if(dev == NULL)
		return GSG_ERROR;

	if(!dev->mounted)
		return GSG_ERROR;

	if(address > dev->endAddress)
		return GSG_INVALID_ARG;
	
	if(length == 0)
		return GSG_INVALID_ARG;

	if(length > (dev->endAddress - address + 1))
		return GSG_INVALID_ARG;

	if(data == NULL)
		return GSG_ERROR_NULL_POINTER;
		
	if((dev->dummy) && (dev->dummyMemPtr == NULL))
		return GSG_ERROR_NULL_POINTER;

	if(getNvmLock(dev, accType) != 1)
		return GSG_BUSY;

	if(accType == NVM_WRITE)
	{
		dev->busy = 2;
		// Here we assume that read and write functions are already present
		// Handle page rollover
		nvm_address_t writeAddress = address;
		uint32_t remainingLength = length;
		nvm_data_t *writeDataPtr = data;

		if(dev->handlePageRollOver)
		{
			while(remainingLength > 0)
			{
				uint32_t nextPageBoundary = ((writeAddress / dev->pageSize) + 1) * dev->pageSize;
				uint16_t writeLength = (remainingLength > (nextPageBoundary - writeAddress)) ? 
										(nextPageBoundary - writeAddress) : remainingLength;

				if(dev->dummy)
				{
					for(uint16_t i = 0; i < writeLength; i++)
						*(((nvm_data_t *)dev->dummyMemPtr) + writeAddress + i) = *(writeDataPtr + i);
				}
				else
				{
					if(dev->writeFn(dev->context, writeAddress, writeDataPtr, writeLength) != GSG_SUCCESS)
					{
						releaseNvmLock(dev);
						return GSG_ERROR;
					}
				}

				if(dev->postWriteDelayMs > 0)
					vTaskDelay(pdMS_TO_TICKS(dev->postWriteDelayMs));

				remainingLength -= writeLength;
				writeAddress += writeLength;
				writeDataPtr += writeLength;
			}
		}
		else
		{
			// Page rollover handling not enabled, single write
			if(dev->dummy)
			{
				for(uint16_t i = 0; i < remainingLength; i++)
					*(((nvm_data_t *)dev->dummyMemPtr) + writeAddress + i) = *(writeDataPtr + i);
			}
			else
			{
				if(dev->writeFn(dev->context, writeAddress, writeDataPtr, remainingLength) != GSG_SUCCESS)
				{
					releaseNvmLock(dev);
					return GSG_ERROR;
				}
			}

			if(dev->postWriteDelayMs > 0)
				vTaskDelay(pdMS_TO_TICKS(dev->postWriteDelayMs));
		}
	}
	else if(accType == NVM_READ)
	{
		dev->busy = 1;
		if(dev->dummy)
		{
			for(uint16_t i = 0; i < length; i++)
				*(data + i) = *(((nvm_data_t *)dev->dummyMemPtr) + address + i);
		}
		else
		{
			if(dev->readFn(dev->context, address, data, length) != GSG_SUCCESS)
			{
				releaseNvmLock(dev);
				return GSG_ERROR;
			}
		}

		if(dev->postReadDelayMs > 0)
			vTaskDelay(pdMS_TO_TICKS(dev->postReadDelayMs));
	}
	dev->busy = 0;
	releaseNvmLock(dev);
	return GSG_SUCCESS;
}

gsg_result_t NVM_writeData(nvm_device_t *dev, nvm_address_t address, nvm_data_t *data, uint16_t length)
{
	if(accessNvmCell(dev, address, data, length, NVM_WRITE) == GSG_SUCCESS)
		return GSG_SUCCESS;
	else
		return GSG_ERROR;
}

gsg_result_t NVM_readData(nvm_device_t *dev, nvm_address_t address, nvm_data_t *data, uint16_t length)
{
	if(accessNvmCell(dev, address, data, length, NVM_READ) == GSG_SUCCESS)
		return GSG_SUCCESS;
	else
		return GSG_ERROR;
}

gsg_result_t NVM_Init(nvm_device_t *dev)
{
	if(dev == NULL)
		return GSG_ERROR_NULL_POINTER;

	if(dev->size == 0)
	 	return GSG_ERROR;
	
	if(dev->endAddress != (dev->startAddress + dev->size - 1))
		return GSG_ERROR;

	if(dev->pageSize == 0)
		return GSG_ERROR;

	if(dev->dummy)
	{
		dev->dummyMemPtr = (uint8_t *)calloc(1, dev->size);
		if(dev->dummyMemPtr == NULL)
			return GSG_ERROR;
	}

	if(!dev->dummy && (dev->readFn == NULL || dev->writeFn == NULL))
		return GSG_ERROR;

	#if (GSG_OS_USED == GSG_OS_FREERTOS)	//freeRTOS
	dev->nvmMutex = xSemaphoreCreateMutex();
	if(dev->nvmMutex == NULL)
		return GSG_ERROR;

	#if NVM_ENABLE_PERIODIC_PAGE_WRITE
	dev->nvmWriteTimerHandle = xTimerCreate("NvmPgWr", pdMS_TO_TICKS(NVM_PAGE_WRITE_INTERVAL_MS), pdTRUE, (void *)dev, nvmPageWriteTimerCallback);
	if(dev->nvmWriteTimerHandle == NULL)
	{
		vSemaphoreDelete(dev->nvmMutex);
		return GSG_ERROR;
	}
	#endif
	#endif

	/* Mount the device only if all initialization succeeded */
	dev->mounted = 1;
	dev->busy = 0;
	return GSG_SUCCESS;
}

gsg_result_t NVM_DeInit(nvm_device_t *dev)
{
	if(dev == NULL)
		return GSG_ERROR_NULL_POINTER;

	/* Unmount the device first */
	dev->mounted = 0;

	#if (GSG_OS_USED == GSG_OS_FREERTOS)	//freeRTOS
	#if NVM_ENABLE_PERIODIC_PAGE_WRITE
	if(dev->nvmWriteTimerHandle != NULL)
	{
		xTimerDelete(dev->nvmWriteTimerHandle, pdMS_TO_TICKS(1000));
		dev->nvmWriteTimerHandle = NULL;
	}
	#endif

	if(dev->nvmMutex != NULL)
	{
		vSemaphoreDelete(dev->nvmMutex);
		dev->nvmMutex = NULL;
	}
	#endif

	if(dev->dummy && dev->dummyMemPtr != NULL)
	{
		free(dev->dummyMemPtr);
		dev->dummyMemPtr = NULL;
	}

	return GSG_SUCCESS;
}

// NVM Page write timer handle
static void nvmPageWriteTimerCallback(TimerHandle_t xTimer)
{
	;
}



#endif
