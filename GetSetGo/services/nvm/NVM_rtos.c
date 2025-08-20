/*
 * NVM_rtos.c
 *
 *  Created on: Dec 23, 2024
 *      Author: antiq
 */

#include "NVM.h"

//static char hex[100];

uint8_t hexLength;
static uint8_t moduleInit = 0;

#if (GSG_USE_RTOS == 1)	//freeRTOS
	static QueueHandle_t rqstQueueHandle;
	TaskHandle_t nvmTaskHandle;
	static void nvmRqstHandlerTask(void *pvParameters);
#endif

bool NVM_Write_Cell(nvm_address_t Address, nvm_data_t Byte);
bool NVM_Read_Cell(nvm_address_t Address, nvm_data_t *ptr);
uint8_t NVM_Write_Sequence(nvm_address_t Address, nvm_data_t *String, uint16_t Length);
uint8_t NVM_Read_Sequence(nvm_address_t Address, nvm_data_t *String, uint16_t Length);


void NVM_Init(void)
{
	#ifdef NVM_USE_DUMMY_NVM
		memset(NVM_DUMMY_MEMORY,0xFF,sizeof(NVM_DUMMY_MEMORY));
	#endif

#if (GSG_USE_RTOS == 1)	//freeRTOS
	rqstQueueHandle = xQueueCreate( NVM_RQST_QUEUE_LENGTH , sizeof(nvm_rqst_t) );
	configASSERT( xTaskCreate(nvmRqstHandlerTask, 	"NVM", 	2048, NULL, NVM_TASK_PRIORITY, &nvmTaskHandle) );
#endif

}

#if (GSG_USE_RTOS == 1)	//freeRTOS
static void nvmRqstHandlerTask(void *pvParameters)
{
	BaseType_t status=pdFALSE;
	UBaseType_t uxHighWaterMark;
	//uxHighWaterMark = uxTaskGetStackHighWaterMark( NULL );

	nvm_rqst_t rqstData;
	//nvm_data_t rdData = NVM_CELL_RST_VALUE;
	while(1)
	{
		if(moduleInit == 0)
		{	SEGGER_SYSVIEW_PrintfHost("Init");
			if(xTaskNotifyWait(0, 0,NULL, portMAX_DELAY) == pdTRUE)  //ToDo: Recheck this value
				;
			//Any need to init NVM?
			moduleInit = 1;
		}
		else
		{
			if(xQueueReceive(rqstQueueHandle, &rqstData, portMAX_DELAY) == pdTRUE)
			{
				//(rqstData.type == NVM_READ)?SEGGER_SYSVIEW_PrintfHost("RqRd %d",rqstData.address):SEGGER_SYSVIEW_PrintfHost("RqWr %d",rqstData.address);

				//perform the NVM operation
				if(rqstData.type == NVM_WRITE)
				{
					if(rqstData.dataPtr != NULL)
					{
						status = NVM_Write_Sequence(rqstData.address,rqstData.dataPtr, rqstData.length);
						vTaskDelay(pdMS_TO_TICKS(6));// give it some time
					}
					else
					{
						SEGGER_SYSVIEW_PrintfHost("wrNULL");
						continue;	//drop this request
					}


				}
				else if(rqstData.type == NVM_READ)
				{
					//Save the read data
					if(rqstData.dataPtr != NULL)
					{
						status = NVM_Read_Sequence(rqstData.address,rqstData.dataPtr, rqstData.length);
					}
					else
					{
						SEGGER_SYSVIEW_PrintfHost("rdNULL");
						continue;	//drop this request
					}


					#if (NVM_USE_BUFFER_POOL==1)
					else
					{
						;	//Will add buffer pool code later
					}
					#endif
				}

				//Notify the requesting task along with the status of operation
				xTaskNotify(rqstData.task,status,eSetValueWithOverwrite);

				#if (GSG_USE_SEGGER_TRACE == 1)
				if (status != pdTRUE )
				{
					if(rqstData.type == NVM_READ)
						SEGGER_SYSVIEW_PrintfHost("rdErr@%X", rqstData.address);
					else
						SEGGER_SYSVIEW_PrintfHost("wrErr@%X", rqstData.address);
				}
				else
				{
					/*if(rqstData.type == NVM_READ)
						SEGGER_SYSVIEW_PrintfHost("rd0x%X=%X", rqstData.address,rdData);
					else
						SEGGER_SYSVIEW_PrintfHost("wr0x%X=%2X", rqstData.address,*rqstData.dataPtr);*/
				}
				#endif
			}
		}
	}
}
#endif


/**
 * This function sends a request to the NVM task for either reading or writing data.
 * It populates an nvm_rqst_t structure with the required parameters and places it
 * into the request queue. The requesting task is expected to handle synchronization
 * via task notifications or other mechanisms.
 */
BaseType_t NVM_rqstData(TaskHandle_t task, nvm_address_t address, void *destPtr, uint16_t length, nvm_rqst_type_t type)
{
    nvm_rqst_t rqstData;

    //Early detection of failure
    if(address >= NVM_Max_Valid_Address)
    {
    	//sprintf(hex,"\nFAULT@%d",(uint16_t)address);DEBUG_sendHex;
    	return FAIL;
    }


    // Populate the structure fields
    rqstData.address 	= address;    // Address in the NVM
    rqstData.dataPtr 	= destPtr;    // Pointer to the data buffer
    rqstData.length 	= length;      // Length of data to read/write
    rqstData.type 		= type;         // Enum for request type (read/write)

#if (GSG_USE_RTOS == 1)	//freeRTOS

    // Retrieve the current task handle if it's NULL
	if (task == NULL)
		task = xTaskGetCurrentTaskHandle();
	rqstData.task 		= task;          // Handle of the requesting task

	// Send the request to the queue
    if (xQueueSend(rqstQueueHandle, &rqstData, 0) == pdPASS)
    {
        return pdPASS; // Successfully queued the request
    }
#endif

    return pdFAIL; // Failed to queue the request
}

