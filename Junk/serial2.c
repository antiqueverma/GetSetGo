#include "serial.h"

struct serial_tx SER_TX;
struct serial_rx SER_RX;
enum serial_state SER_STATE;
const unsigned char SER_RX_TERMINATOR[] = SER_RX_TERMINATOR_STR;


QueueHandle_t	qhSER_Rx_Queue;
QueueHandle_t	qhSER_Tx_Queue;
TaskHandle_t	* const thSER_RX_TASK;
TaskHandle_t	* const thSER_TX_TASK;


//Reception ISR also adds the received NULL character in the RX_BUFF
#ifdef	SER_USE_RX_ISR
void SER_RX_IRQ_HANDLER (void)
{
		static unsigned char trm1_rcvd;
#ifdef SER_IMP_FREERTOS
		BaseType_t	xStatus = pdPASS;
		SER_RX.BUFF[SER_RX.ToDo] = SER_DREG;  //Transfer data to Serial RX Buffer
		if(SER_RX.RX_BLOCK == UNBLOCK)  //If Reception is unblocked
		{
			if(UART_DREG == SER_RX_TERMINATOR[0]) //End of line was received
			{
				trm1_rcvd = 0xFF;
			}
			if(UART_DREG == SER_RX_TERMINATOR[1] && trm1_rcvd)
			{
				if(SER_RX.ToDo > 1)
				{
					SER_RX.BUFF[SER_RX.ToDo-1] = 0x00;		//Put NULL at position of \r
					if(SER_RX.BYPASS_Q)		//If RX Queue is to be bypassed, do not transfer data to SER_RX_Queue
					{
						SER_RX.M_FLAG = 1;
					}
					else
					{
						xStatus = xQueueSendFromISR(qhSER_Rx_Queue,SER_RX.BUFF, 0);
						if(xStatus != pdPASS)
							PostFAULT_fromISR(FAULT_SER_RX_Q_FULL);//SER_TxString("#Prob\n");
					}

				}
				SER_RX.ToDo = 0;
				trm1_rcvd = 0x00;
			}
			else
				SER_RX.ToDo++;
		}
//		SER_RX.BUFF[SER_RX.ToDo] = 0x00;  //Reset the next location
		if(SER_RX.ToDo >= SER_RX_BUFF_SIZE)		//Circular buffer might cause issue with RTOS queue
			SER_RX.ToDo = 0;
#endif

#ifdef SER_IMP_PROCEDURAL
	SER_RX.BUFF[SER_RX.ToDo] = UART_DREG;  //Data register should always be checked for every Rx Packet regardless of whether the packet is wanted or not
	if(SER_RX.RX_BLOCK == UNBLOCK)
	{
		if(UART_DREG == SER_RX_TERMINATOR[0])
		{
			SER_RX.M_FLAG = 1;
			SER_RX.B_FLAG = 1;
		}
		else
		{
			SER_RX.B_FLAG = 0x01;
		}
		SER_RX.ToDo++;
	}
	SER_RX.BUFF[SER_RX.ToDo] = 0x00;
	if(SER_RX.ToDo > SER_RX_BUFF_SIZE)
		SER_RX.ToDo = 0;
#endif

}
#endif

#ifdef	SER_IMP_FREERTOS
void SER_Use_Rx_Queue(unsigned char stat)		//Pass 0 to bypass Rx Queue
{
	if(stat)
		SER_RX.BYPASS_Q = 0;		//Use Queue
	else
		SER_RX.BYPASS_Q = 1;		//Bypass Queue
}

char *SER_Get_Rx_Data(void)
{
	SER_RX.M_FLAG = 0;
	return SER_RX.BUFF;
}

/*void SER_Rx_Force_Terminate(void)
{
	;
}*/

void tSER_RX_TASK(void *pvParameters)
{
	//char *message;
	BaseType_t	xStatus;
	unsigned char RxData[SER_RX_BUFF_SIZE],ptr;
	while(1)
	{
		#if (SER_RX_PROCESSING_LEVEL == 0)
		if(SER_RX.M_FLAG)
		{	SER_TxString("#A.1 ");
			ptr = 0;
			while (SER_RX.ToDo != SER_RX.Done)  //If data is available, transfer it to queue
			{
				RxData[ptr] = SER_RX.BUFF[SER_RX.Done];
				SER_RX.Done++;
				ptr++;

				if(SER_RX.Done >= SER_RX_BUFF_SIZE)  //This ensures that SER_RX.Buff is a circular buffer
					SER_RX.Done = 0;
			}
			RxData[ptr] = '\0';
			xStatus = xQueueSend(qhSER_Rx_Queue,RxData, 0);
			if(xStatus != pdPASS)
				;
			else
				SER_TxString("#A.2 ");
			SER_RX.M_FLAG = 0;
		}
		vTaskDelay(pdMS_TO_TICKS(SER_INBOX_QUERY_PERIOD));
		#endif
	}
}

void tSER_TX_TASK(void *pvParameters)
{
	BaseType_t	xStatus;
	//const TickType_t xTicksToWait = 0;
	char TxData[SER_TX_BUFF_SIZE];
	while(1)
	{
		#if (SER_RX_PROCESSING_LEVEL == 0)
		//if(SER_TX.M_FLAG)
		{
			while(uxQueueMessagesWaiting(qhSER_Tx_Queue) != 0)
			{
				SER_TxString("#C ");
				xStatus = xQueueReceive(qhSER_Tx_Queue,TxData, 0);
				if(xStatus == pdPASS)
					SER_TxBuffer(TxData, New_Send);
				else
					;
			}
			//SER_TX.M_FLAG = 0;
		}
		#endif
		vTaskDelay(pdMS_TO_TICKS(SER_OUTBOX_QUERY_PERIOD));
	}
}
#endif

#ifdef SER_IMP_PROCEDURAL				//Serial Engine
/*void SER_Engine(void)
 {  //unsigned char lcl_rx_ptr;
	if(SER_RX.M_FLAG)
	{
        SER_TxString(SER_RX.BUFF);
		SER_TxByte('\r');
		SER_TxByte('\n');
		SER_RxRESET();
		SER_RX.M_FLAG = 0x00;
    }
    switch(SER_STATE)
	{
		case S_IDLE:
		{
			if(SER_RX.ToDo != SER_RX.Done)
            {	SER_RX.B_FLAG = 0x0;
                SER_STATE = RX_ING;
            }
			else if(SER_TX.CMD2TX < (CMD_NUM+1) )
            {
                SER_STATE = TX_ING;
            }
			break;
		}

		case RX_ING:
		{
			if(SER_RX.ToDo != SER_RX.Done)
			{
                SER_RX.BUFF[SER_RX.Done] = SEC_byteCoding(SER_RX.BUFF[SER_RX.Done]);
                //SER_TxByte(SER_RX.BUFF[SER_RX.Done]);
				SER_RX.B_FLAG = 0x0;
                SER_RX.Done++;
            }
            if(SER_RX.BUFF[SER_RX.Done-1] == 0x00)
			{
				//SER_RX.TimeOut = 0x01;
				SER_STATE = DX_ING;
			}
			break;
		}

		case DX_ING:		//Intermediate state
		{
			switch(CMD_Engine(SER_RX.BUFF))
			{
				case 0x00:	//No parameter received
				{
					//SER_TxString("#A ");
					SER_RX.M_FLAG = 1;
					break;
				}
				case 0x01:	 //A string is received
				{
					//SER_TxString("#B ");
					SER_RX.M_FLAG = 1;
					break;
				}
				case 0x02:	//A number is received
				{
					//SER_TxString("#C ");
					DATA.T1MEAS1 = CMD_getInt();
					SER_RX.M_FLAG = 1;
					break;
				}
				default:
				{
					SER_RX.M_FLAG = 0;
					break;
				}
			}
            
			SER_STATE = S_RESET;
			//SER_RX.TimeOut = 0x00;
			break;
		}

		case TX_ING:
		{
            SER_TX.exPTR = CMD_getCMD(1, SER_TX.CMD2TX);
            SER_TxBuffer(SER_TX.exPTR, New_Hold);
            SER_TxBuffer("\r\n\0",App_Send);
            SER_STATE = S_RESET;
			break;
		}

		case S_RESET:
		{
			SER_Reset();
            SER_STATE = S_IDLE;
			break;
		}

		default:
			break;
	}
}*/
#endif

void SER_Init(void)
{

	#if	SER_IMP_FREERTOS==1
		qhSER_Rx_Queue = xQueueCreate(SER_RX_QUEUE_SIZE,sizeof(SER_RX.BUFF));
		//qhSER_Tx_Queue = xQueueCreate(SER_TX_QUEUE_SIZE,sizeof(SER_TX.BUFF));
		//xTaskCreate(tSER_RX_TASK, NULL, 1000, NULL, SER_RX_TASK_PRIORITY, thSER_RX_TASK);	//Reciever task is the ISR itself
		//xTaskCreate(tSER_TX_TASK, NULL, 1000, NULL, SER_TX_TASK_PRIORITY, thSER_TX_TASK);
		NVIC_SetPriority(USART1_IRQn, configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY);
	#else
		QUE_Init(qSER_RX_Queue,);
	#endif

	SER_RX.ToDo = 0x00;
	SER_RX.Done = 0x00;
	SER_RX.M_FLAG = 0x00;
	SER_RX.TimeOut = 0x00;
	SER_RX.BYPASS_Q = 0;
	SER_Reset();
}

void SER_Reset(void)
{
	//Reset Receiver module
    SER_RX.ToDo = 0x00;
    SER_RX.Done = 0x00;
    SER_RX.TimeOut = 0x00;
    SER_RX.M_FLAG = 0x00;
    SER_RX.RX_BLOCK = UNBLOCK;
    memset(SER_RX.BUFF,0x00,sizeof(SER_RX.BUFF));

    //Reset transmitter module
    memset(SER_TX.BUFF,0x00,sizeof(SER_TX.BUFF));
    SER_TX.PTR = 0x00;
   // SER_TX.exPTR = 0x00;

#ifdef SER_IMP_PROCEDURAL
    //SER_TX.CMD2TX = MAX_CMD+1;

	SER_STATE = S_IDLE;
#endif
}

void SER_Switch_Rx(unsigned char status)
{
	if(status)
		SER_RX.RX_BLOCK = UNBLOCK;
	else
		SER_RX.RX_BLOCK = BLOCK;
}

/****************************************************************************************************************************************
 *Transmission Buffer function that prepares the SERIAL Buffer for transmission.
 *This function updates the SER_TX_BUFF by inserting new/additional data into the buffer and finally sends the buffer contents.
 * Parameters
 *		tx_str[] ->	The string to be inserted in UART transmission buffer
 *		oh_flg	:	0 -> Provided data is fresh data but incomplete, hence do not send
 *					1 -> Provided data is fresh and complete data, hence will be sent
 *					2 -> Provided data is only to be appended in buffer, hence do not send
 *					3 -> Provided data is final data to be appended, Buffer will be transmited finally
 *
 *The user must provide strings in tx_str[] having NULL in the end.
 *However, this function removes all NULLs and manually appends a NULL in the end just before transmitting
 *****************************************************************************************************************************************/
/*void SER_TxBuffer(char *tx_str, char status)
{
	if( (status == New_Hold) || (status == New_Send))
		SER_TX.PTR = 0;
	while((*tx_str) != '\0')		//Detect NULL character, but do not copy that NULL character
	{
    	SER_TX.BUFF[SER_TX.PTR] = (*tx_str);			//Copying each value of tx_str into UART_TX_BUFF
    	SER_TX.PTR++;
    	tx_str++;
	}
	if((status == New_Send) || (status == Add_Send) )
	{
		#ifdef SER_TX_USE_TERMINATORS		//Todo Use logic to transfer  #define string into Tx Buffer
		SER_TX.BUFF[SER_TX.PTR] = '\r';
		SER_TX.PTR++;
		SER_TX.BUFF[SER_TX.PTR] = '\n';
		SER_TX.PTR++;
		#endif
		//Appending additional characters
        //Apply Encryption here
		SER_TX.BUFF[SER_TX.PTR] = '\0'; 	//NULL character for UART library
		SER_TxString(SER_TX.BUFF);
		SER_TX.PTR = 0;
	}
}*/

void SER_TxBuffer(char *tx_str, char status)
{
	SER_TxString(tx_str);
	if(status ==  TX_TERMINATE)
	{
		SER_TxString(SER_TX_TERM_STR);
	}
}

/*************************************************************************************
 * String transmission function
 * Transmits each character of the provided string until a NULL character is detected
 ************************************************************************************/
void SER_TxString (char *string)		//This string function also sends the NULL character '\0'
{
//	unsigned char ptr=0,len=0;
//	char *str;
//	str = string;
	while( (*string) != '\0')
	{
		SER_TxByte(*string);
//		len++;
		string++;
	}
//	string = str;
//	while( ptr < len)				//Detect end of string
//	{
//
//		string++;
//		ptr++;
//	}
}

/***********************************
 * Transmits a provided character  *
 ***********************************/
void SER_TxByte(unsigned char tx_data)
{
   UART_TxBYTE(tx_data);
}

/*
void DEBUG_SerState(void)
{
    static unsigned int prev_st,curr_st;
    curr_st = SER_STATE;
    if(prev_st != curr_st)
    {
        prev_st = curr_st;
		SER_TxString("\r\nSER:");
        switch(SER_STATE)
        {
            case S_IDLE:
                SER_TxString("IDL");
                break;
            case RX_ING:
                SER_TxString("RX");
                break;
            case TX_ING:
                SER_TxString("TX");
                break;
            case DX_ING:
                SER_TxString("DX");
                break;
            case EX_ING:
                SER_TxString("EX");
                break;
        }
    }

}*/
