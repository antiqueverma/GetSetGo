/*
 * FAULT.h
 *
 *  Created on: Jun 4, 2022
 *      Author: antiq
 */

#ifndef DRIVERS_FAULT_H_
#define DRIVERS_FAULT_H_

	#include "main.h"

#ifdef INC_FREERTOS_H
	#define FAULT_QueueSize				10
	extern QueueHandle_t				qhFAULT_Queue;
	#define PostFAULT_fromISR(X)		xQueueSendFromISR(qhFAULT_Queue,X,0)
	#define PostFAULT(X)				X //xQueueSend(qhFAULT_Queue,X,0)

	//Fault Codes
	#define FAULT_SER_RX_Q_FULL			0
	#define FAULT_ESP_INIT_NACK			1










#endif
	void FAULT_Init(void);
#endif /* DRIVERS_FAULT_H_ */
