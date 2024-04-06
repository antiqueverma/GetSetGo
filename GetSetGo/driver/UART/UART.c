/*
 * UART.c
 *
 * Any terminators to be appended is not supported by this library. It should be done by higher layers.
 * Please update system specific terminators in the ISR
 *
 * Created: 17-03-2021 16:18:49
 *  Author: antiq
 */

#include	"UART.h"
//Constant Macros


//UART Configuration Constants
#define TXC_FLAG	BIT5
#define TXEN	BIT3
#define RXEN    BIT4
#define RXFLAG  BIT7
#define RXIE    BIT7
#define RX_COMPLETE_INTERRUPT         (1<<RXCIE0)
#define UART_TRM	'\0'

//UART Buffers and respective queue handlers
#ifdef	UART0_RX_BUFF_SIZE
static uint8_t 				buffRx0[UART0_RX_BUFF_SIZE];	// Global Reception Buffer
static gsg_queueObject_t	qobjRx0;
#endif
#ifdef	UART0_TX_BUFF_SIZE
static uint8_t 				buffTx0[UART0_TX_BUFF_SIZE];	// Global Reception Buffer
static gsg_queueObject_t	qobjTx0;
#endif

#ifdef	UART1_RX_BUFF_SIZE
static uint8_t 				buffRx1[UART1_RX_BUFF_SIZE];	// Global Reception Buffer
static gsg_queueObject_t	qobjRx1;
#endif
#ifdef	UART1_TX_BUFF_SIZE
static uint8_t 				buffTx1[UART1_TX_BUFF_SIZE];	// Global Reception Buffer
static gsg_queueObject_t	qobjTx1;
#endif

#ifdef	UART2_RX_BUFF_SIZE
static uint8_t 				buffRx2[UART2_RX_BUFF_SIZE];	// Global Reception Buffer
static gsg_queueObject_t	qobjRx2;
#endif
#ifdef	UART2_TX_BUFF_SIZE
static uint8_t 				buffTx2[UART2_TX_BUFF_SIZE];	// Global Reception Buffer
static gsg_queueObject_t	qobjTx2;
#endif

#ifdef	UART3_RX_BUFF_SIZE
static uint8_t 				buffRx3[UART3_RX_BUFF_SIZE];	// Global Reception Buffer
static gsg_queueObject_t	qobjRx3;
#endif
#ifdef	UART3_TX_BUFF_SIZE
static uint8_t 				buffTx3[UART3_TX_BUFF_SIZE];	// Global Reception Buffer
static gsg_queueObject_t	qobjTx3;
#endif

#ifdef	UART4_RX_BUFF_SIZE
static uint8_t 				buffRx4[UART4_RX_BUFF_SIZE];	// Global Reception Buffer
static gsg_queueObject_t	qobjRx4;
#endif
#ifdef	UART4_TX_BUFF_SIZE
static uint8_t 				buffTx4[UART4_TX_BUFF_SIZE];	// Global Reception Buffer
static gsg_queueObject_t	qobjTx4;
#endif










//Baud Rates
const long int BAUD_RATE[SUPP_BAUD_RATES][2] =	{{9600,00},
												{19200,00},
												{38400,00},
												{57600,00},
												{76800,00},
												{115200,0x02D9}};

//Terminator reception flags
//char trmRx_flg=0;			//Terminator reception flags for receiving mode







/**********************************************************************************************************************************
 * UART Initialization function (#device dependent)
 * Parameters:
 *	baud_rate -> Pass the required baud rate to be used. If baud_rate is not found in baud array, default 9600bps will be used
 * 	parity : Define Parity to be used
 * 		0 -> No parity
 * 		1 -> Even parity
 ***********************************************************************************************************************************/
void UART_INIT(scomObject_t *objUART,
		uint8_t instance,
		uint32_t baud_rate,
		uint8_t parity,
		uint8_t rxen)     //Use 8 bits, even parity and 2 stop bits
{
	objUART->rxbyte = NULL;
	objUART->txbyte = NULL;

	#if(SYS_MCU_SERIES == MCU_SERIES_STM32)
		int br_unident=1;
		RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN ;//(1<<0);  // Enable the GPIOA clock
		RCC->APB2ENR |= (RCC_APB2ENR_USART1EN);
		//Below three lines should be called in App function
		GPIOA->MODER |= GPIO_MODER_MODER9_1 | GPIO_MODER_MODER10_1;		//This is for USART1
		GPIOA->OSPEEDR |= (3UL<<18)|(3UL<<20);
		GPIOA->AFR[1] |= (7UL<<4)|(7UL<<8);

		switch(instance)
		{
			case 1:
			{
				USART1->CR1 = 0x00000000;
				USART1->CR2 = 0x00000000;
				USART1->BRR = (0x09<<0) | (0x2D<<4);//115200 default Baud Rate
				for (int x=0;x<SUPP_BAUD_RATES;x++)
				{
					if(baud_rate == (BAUD_RATE[x][0]))
					{
						USART1->BRR = (unsigned int) BAUD_RATE[x][1];
						objUART->speed = BAUD_RATE[x][0];
						br_unident=0;
					}
				}
				if(br_unident)		//Default setting
					USART1->BRR = 0x02D9;	// Setting BAUD rate to 115200

				if(parity == UART_EVEN_PARITY)
					USART1->CR1 |= USART_CR1_PCE;	//Defaul setting for PS is
				else if(parity == UART_ODD_PARITY)
					USART1->CR1 |= USART_CR1_PCE| USART_CR1_PS;
				else
					USART1->CR1 &= ~USART_CR1_PCE;

				USART1->CR1 |= USART_CR1_RXNEIE | USART_CR1_TE | USART_CR1_RE; //Enable the transmitter AND Receiver with Receiver interrupt
				USART1->CR1 &= ~USART_CR1_M;
				USART1->CR1 |= USART_CR1_UE;

				if(rxen)
				{
					NVIC_EnableIRQ(USART1_IRQn);
					QUE_Init(&qobjRx1, &buffRx1, sizeof(buffRx1), sizeof(uint8_t), QUE_TYP_FIFO);
					objUART->rxbyte = UART_RxBYTE;
				}

				objUART->txbyte = UART_TxBYTE;


				break;
			}
		}
		__enable_irq();

	#endif
	
}

/*****************************************************************
 * Baud rate querying function (#device dependent)
 * Returns the baud_rate currently under utilisation
 * Performs lookup in the baud array
 *****************************************************************/
/*long UART_querBAUD()
{int x;
	for (x=0;x<SUPP_BAUD_RATES;x++)
		if(USART1->BRR == (BAUD_RATE[x][1]))
			break;
	return BAUD_RATE[x][0];
}*/


//Not working as expected due to hardware aspects, hence calling is not recommended #device dependent
/*int UART_setBAUD(long int baud_rate)
{int br_unident=1;
	for (int x=0;x<SUPP_BAUD_RATES;x++)
	{
		if(baud_rate == (BAUD_RATE[x][0]))
		{UCSR0B	&= ~(TXEN | RXEN);	//Stop  the Tx Rx modules
		 UBRR0H	= 0;
		 UBRR0L	= BAUD_RATE[x][1];
		 br_unident=0;
		 UCSR0B	|= TXEN | RXEN;}	//Start the TX RX modules
	}
	if(br_unident)		//Default setting
		{UCSR0B	&= ~(TXEN | RXEN);	//Stop  the Tx Rx modules
		 UBRR0H	= 0;
		 UBRR0L	= 103;	// Setting BAUD rate to default 9600
		 UCSR0B	|= TXEN | RXEN;	//Start the TX RX modules
		 return 0;}		//Return 0 to indicate that Baud rate setting was unsuccessful
	else				//Baud rate setting was successful
		return 1;		//Return 1 to indicate that Baud rate setting was successful
}*/

/*******************************************************************************************************
 * Baud rate checking function
 * Pass a required baud rate to check if it is supported by device or not and function will return flag
 * 	0 -> Baud rate not found
 *	1 -> Baud rate was found
 * Works according to baud array
 *******************************************************************************************************/
/*unsigned char UART_chkBAUD(long int baud_rate)
{
	int br_ident=0;		//0-> Unidentified baudrate    1 -> Identified baudrate
	for (int x=0;x<SUPP_BAUD_RATES;x++)
	{
		if(baud_rate == (BAUD_RATE[x][0]))
			{br_ident=1; break;}
	}
	if(br_ident == 0)		//Default setting
		return 0;		//Return 0 to indicate that Baud rate was not found
	else
		return 1;		//Return 1 to indicate that Baud rate was found
}*/


/*************************************************************************************
 * String transmission function
 * Transmits each character of the provided string until a NULL character is detected
 ************************************************************************************/
void UART_TxSTRING (uint8_t instance, uint8_t *string)		//This string function also sends the NULL character '\0'
{	
	while( (*string) != '\0')
		UART_TxBYTE(instance, *string);
}

/****************************************************************************************************************************************
 *UART Tx function to send a fixed length of string (including any character)
 * Parameters
 *		tx_str[] ->	The string to be transmitted
 *****************************************************************************************************************************************/
void UART_TxBUFFER(uint8_t instance, uint8_t tx_str[], uint16_t length)
{
	uint16_t	i;
	for(i=0 ; i<length ; i++)
	{
		UART_TxBYTE(instance, tx_str[i]);
	}
}

/*************************************************************************************
 * Byte transmission function (#device dependent)
 * Transmits a provided character
 ************************************************************************************/
bool UART_TxBYTE(uint8_t instance, uint8_t tx_data)
{
	#if(SYS_MCU_SERIES == MCU_SERIES_STM32)
	switch(instance)
	{
		case 1:
		{
			while(!(USART1->SR & USART_SR_TC));
				USART1->DR = tx_data;
			break;
		}
		case 2:
		{
			while(!(USART2->SR & USART_SR_TC));
				USART2->DR = tx_data;
			break;
		}
		default:
		{
			return FAIL;
		}
	}

	return PASS;
	#endif
}

bool UART_RxBYTE(uint8_t instance, uint8_t *rx_data)
{
	switch(instance)
	{
		case 1:
		{
			if(QUE_Delete(&qobjRx1,&rx_data))
				return PASS;
			else
				return FAIL;
			break;
		}
		default:
		{
			return FAIL;
		}
	}

}


/*********************************************************************************
 * Call this function repeatedly in main to check for any inbox messages
 * Replace the function CMD_RX() with system specific message responding function
 *********************************************************************************/
/*int UART_INBOX()
{
	if(UART_RX_FLAG == 1)		//New message in INBOX
	{	CMD_RX(UART_RX_BUFF);	//Call message reader
		UART_RX_FLAG = 0;		//Message has been read
		return 1;				//Return 1 to indicate that task is done
	}
	else
	return 0;	//No message received
}*/





