/*
 * FAULT.c
 *
 *  Created on: Jun 4, 2022
 *      Author: antiq
 */
#include "FAULT.h"

QueueHandle_t	qhFAULT_Queue;
//unsigned char FAULT_temp;

void FAULT_Init(void)
{
	qhFAULT_Queue = xQueueCreate(FAULT_QueueSize,sizeof(char));
}
