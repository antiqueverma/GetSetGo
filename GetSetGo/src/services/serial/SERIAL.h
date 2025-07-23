
#ifndef _SERIAL_H
#define _SERIAL_H

#include "sys_core.h"
#include "SerialConfig.h"


#ifndef	SER_IMP
	#warning Serial Implementation method not defined. Procedural method will be used by default.
	#define SER_IMP 2
#endif

#ifndef	SER_TX_BUFF_SIZE
	#warning Transmission buffer size not defined. 10 Bytes will be used as default size
	#define SER_TX_BUFF_SIZE 10		//Size in bytes
#endif

#ifndef	SER_RX_BUFF_SIZE
	#warning Transmission buffer size not defined. 10 Bytes will be used as default size
	#define SER_RX_BUFF_SIZE 10	//Size in bytes
#endif

#ifdef SER_TX_USE_TERMINATORS
	#ifndef	SER_TX_TERM_STR
		#warning Transmission message terminator not defined. '\0' will be used as default
	#endif
#else
	#define SER_TX_USE_TERMINATORS 0	//Do not use any terminator
#endif

#ifdef SER_USE_RX_ISR
	#if SER_RX_IRQ_HANDLER == 1
		#error Please define a valid Serial reception Handler name
	#endif
#endif

#if	(SER_IMP == 1)
	#define SER_IMP_FREERTOS		//Serial will be based on FreeRTOS

	#ifndef INC_FREERTOS_H
		#error FreeRTOS files not included
	#endif

	#ifndef SER_RX_TASK_PRIORITY
		#warning Serial Receiver task priority not defined. 2 Will be used as default value
		#define	SER_RX_TASK_PRIORITY 2
	#endif

	#ifndef SER_TX_TASK_PRIORITY
		#warning Serial Transmitter task priority not defined. 1 Will be used as default value
		#define	SER_TX_TASK_PRIORITY 1
	#endif

	#ifndef SER_INBOX_QUERY_PERIOD
		#warning Serial Receiver Task interval not defined. 100ms Will be used as default value
		#define	SER_INBOX_QUERY_PERIOD 100
	#endif

	#ifndef SER_OUTBOX_QUERY_PERIOD
		#warning Serial Transmitter Task interval not defined. 500ms Will be used as default value
		#define	SER_OUTBOX_QUERY_PERIOD 500
	#endif

	#ifndef SER_RX_QUEUE_SIZE
		#warning Serial Rx Queue Size not defined. 5(messages) be used as default value
		#define	SER_RX_QUEUE_SIZE 5
	#endif

	#ifndef SER_TX_QUEUE_SIZE
		#warning Serial Tx Queue Size not defined. 5(messages) be used as default value
		#define	SER_TX_QUEUE_SIZE 5
	#endif

	extern QueueHandle_t	qhSER_Rx_Queue;
	extern QueueHandle_t	qhSER_Tx_Queue;
	extern TaskHandle_t	* const thSER_RX_TASK;
	extern TaskHandle_t	* const thSER_TX_TASK;
#elif	(SER_IMP == 2)
	#define SER_IMP_PROCEDURAL	//Serial will be based on procedural style
	void SER_Engine(void);
#else
	#error Please define a valid implementation method for Serial Drivers using constant SER_IMP
#endif

#define SER_BUFF_ENB 1

	typedef struct serial_rx
	{
		unsigned char TimeOut;
		unsigned long INT_DATA;
		unsigned char *STR_DATA;
		unsigned char M_FLAG:1;	//Message Flag
		unsigned char B_FLAG:1;	//Byte Flag
		unsigned char RX_BLOCK:1;
		unsigned char BYPASS_Q:1;	//0 to use RX Queue, 1 to bypass RX_Queue
		unsigned char XTRA:4;
	} serial_rx;


	typedef struct serial_tx
	{
		char BUFF[SER_TX_BUFF_SIZE];
		unsigned char PTR;
		//char *exPTR;
		//unsigned char M_FLAG:1;	//Message Flag
		//unsigned char B_FLAG:1;	//Byte Flag
		//unsigned char CMD2TX;		//To hold Command ID of the command to be sent
	} serial_tx;


	typedef enum serial_state{
		S_IDLE,
		RX_ING,
		TX_ING,
		DX_ING,
		EX_ING,
		S_RESET,
		WAIT_4_TERM
	} serial_state;
	extern struct serial_tx SER_TX;
		extern struct serial_rx SER_RX;
	//Extern variables from other files
	extern enum serial_state SER_STATE;
	#define	New_Hold    		0
	#define	New_Send			1
	#define	Add_Hold			2
	#define Add_Send			3

	#define TX_HOLD				0
	#define TX_TERMINATE		1

	void SER_Init(void);
	void SER_Reset(void);
	void SER_TxByte(unsigned char tx_data);
	void SER_TxBuffer(char tx_str[], char status);
	void SER_TxString (char *string);
	void SER_Switch_Rx(unsigned char status);
	void SER_Use_Rx_Queue(unsigned char stat);
	char *SER_Get_Rx_Data(void);
	void DEBUG_SerState(void);
#endif
