#include "UART.h"
#include "stm32f4xx.h"
#define FREE	0
#define BUSY	0xFF
static unsigned char RESOURCE[MAX_MODS];

struct MODULE{
			volatile unsigned int *DR;
			volatile unsigned int *SR;
			volatile unsigned int *CR1;
			volatile unsigned int *CR2;
			volatile unsigned int *BRR;
			unsigned int IRQn;			
			};

const unsigned long int BAUD_RATE[SUPP_BAUD_RATES] ={9600,115200};
const unsigned int BR_VAL[SUPP_BAUD_RATES] = {0x00,0x02D9};
			
class UART{
	#include "stm32f4xx.h"
	private:
		struct MODULE module;
		static unsigned int RX_BUFF_SIZE;
		static unsigned int TX_BUFF_SIZE;
		
		unsigned char ALLOT_FLAG;
		unsigned long BAUD;
		unsigned char RX_BUFFER[200];
		unsigned char TX_BUFFER[200];
		unsigned char RX_PTR;
		unsigned char TX_PTR;

		//Functions
		void UART_TxBYTE(char tx_data);

	public:
		UART(unsigned char ins);
		unsigned char INIT(long int baud_rate, int parity);     //Use 8 bits, even parity and 2 stop bits
		void TxINT(int num, char status);
		void TxSTRING (unsigned char *string);
		void TxBYTE(char byte);
		void TxBUFFER (unsigned char *string, char status);
		unsigned char chkBAUD(long int baud_rate);
		unsigned long int querBAUD();
		int UART_INBOX();
	public:
};

UART::UART(unsigned char ins)
{
	if(ins >= MAX_MODS) //Demanded module does not exist
	{
		ALLOT_FLAG = 0x00;
		return;
	}
	else if(RESOURCE[ins] == BUSY)	//Module is available but not free
	{
		ALLOT_FLAG = 0x00;
		return;
	}
	else if(RESOURCE[ins] == FREE)	//Module is free
	{
		switch(ins)
		{
			/*case 0:
				module.DR  = &(USART0->DR);
				module.CR1 = &(USART0->CR1);
				module.BRR = &(USART0->BRR);
				module.SR  = &(USART0->SR);

				ALLOT_FLAG = 0xFF;
				RESOURCE[ins]=0xff;
				break;*/

			case 1:
				module.DR		= &(USART1->DR);
				module.CR1 	= &(USART1->CR1);
				module.BRR 	= &(USART1->BRR);
				module.SR		= &(USART1->SR);

				RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN ;//(1<<0);  // Enable the GPIOA clock
				RCC->APB2ENR |= (RCC_APB2ENR_USART1EN);
				GPIOA->MODER |= GPIO_MODER_MODER9_1 | GPIO_MODER_MODER10_1;		//This is for USART1
				GPIOA->OSPEEDR |= (3UL<<18)|(3UL<<20);
				GPIOA->AFR[1] |= (7UL<<4)|(7UL<<8);
				NVIC_EnableIRQ(USART1_IRQn);
				ALLOT_FLAG = 0xFF;
				RESOURCE[ins]= BUSY;
				break;

			case 2:
				module.DR		= &(USART2->DR);
				module.CR1 	= &(USART2->CR1);
				module.BRR 	= &(USART2->BRR);
				module.SR		= &(USART2->SR);

				ALLOT_FLAG = 0xFF;
				RESOURCE[ins]=BUSY;
				break;

			default:	//This is a safety condition, but will never happen
				RESOURCE[ins]= FREE;
				ALLOT_FLAG = 0x00;
				break;
		}
	}
}


/**********************************************************************************************************************************
 * UART Initialization function (#device dependent)
 * Parameters:
 *	baud_rate -> Pass the required baud rate to be used. If baud_rate is not found in baud array, default 9600bps will be used
 * 	parity : Define Parity to be used
 * 		0 -> No parity
 * 		1 -> Even parity
 ***********************************************************************************************************************************/
unsigned char UART::INIT(long int baud_rate, int parity)     //Use 8 bits, even parity and 2 stop bits
{
	//Terminate function if UART module is not available
	if(ALLOT_FLAG == 0x00)	//If not allotment was decided
		return 0;
	else
	{	
		//RESET the configuration registers
		*module.CR1 = 0x00000000;
		*module.CR2 = 0x00000000;
		for (int x=0;x<SUPP_BAUD_RATES;x++)
			{
				if(baud_rate == BAUD_RATE[x])
				{
					*module.BRR = (unsigned int)BR_VAL[x];
					break;
				}
				else 
					*module.BRR = 0x02D9;	// Setting BAUD rate to 115200
			}
		
			if(parity == UART_EvenParity) 
				*module.CR1 |= USART_CR1_PCE;	//Defaul setting for PS is 
			else if(parity == UART_OddParity)
				*module.CR1 |= USART_CR1_PCE| USART_CR1_PS;
				else 
					*module.CR1 &= ~USART_CR1_PCE;	

			
			//*module.BRR = (0x09<<0)|(0x2D<<4);//115200 Baud Rate
			*module.CR1 |= USART_CR1_RXNEIE | USART_CR1_TE | USART_CR1_RE; //Enable the transmitter AND Receiver with Receiver interrupt
			*module.CR1 &= ~USART_CR1_M;
			*module.CR1 |= USART_CR1_UE;			
			__enable_irq();
	}
	return 0xFF;
}

/*****************************************************************
 * Baud rate querying function (#device dependent)
 * Returns the baud_rate currently under utilisation
 * Performs lookup in the baud array
 *****************************************************************/
unsigned long int UART::querBAUD()
{int x;
	for (x=0;x<SUPP_BAUD_RATES;x++)
		if( *module.BRR == BR_VAL[x] )
			break;
	return BAUD_RATE[x];
}


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

/*************************************************************************************
 * Integer transmission function
 * Converts a given integer into a string and then transmits each digit sequentially
 ************************************************************************************/
void UART::TxINT(int num, char status)
{	unsigned char digits[10];
	long int x,y=num,d=1;
	int i=0,f=0;
	for (int z=0 ; y>0 ;z++)
		{d = d*10;
		y = y/10;
		}
	y = num;
	if(y < 0)
		{y = y * (-1);
		digits[i] = '-'; i++;}

	if(y > 0)
		while(d>0)
		{
			x=y/d;
			if((x != 0) || (f == 1))
			{
				f=1;
				digits[i] = x + '0';
				i++;
			}
			y=y-x*d;
			d=d/10;
		}
	else //Execute when number is 0
		{digits[i] = '0'; i++;}
	digits[i]='\0';		//Manually append a NULL Character always
	if(status == UART_New_Send)
		TxSTRING(digits);
	else
		TxBUFFER(digits, status);
}

/*************************************************************************************
 * String transmission function
 * Transmits each character of the provided string until a NULL character is detected
 ************************************************************************************/
void UART::TxSTRING (unsigned char *string)		//This string function also sends the NULL character '\0'
{
	for(unsigned int i=0 ; *(string + i) != '\0' ; i++)
		UART_TxBYTE(*(string+i));
}

/****************************************************************************************************************************************
 *Transmission Buffer function that prepares the UART Buffer for transmission.
 *This function updates the UART_TX_BUFF by inserting new/additional data into the buffer and finally sends the buffer contents.
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
void UART::TxBUFFER(unsigned char *string, char status)
{	int i=0;
	if( (status == 0) || (status == 1))
		TX_PTR = 0;

	while( *(string+i) != '\0')		//Detect NULL character, but do not copy that NULL character
		{
		TX_BUFFER[TX_PTR] = *(string+i);			//Copying each value of tx_str into UART_TX_BUFF
		TX_PTR++;
		i++;
		}

	if((status == 1) || (status == 3) )
	{//Appending additional characters
		TX_BUFFER[TX_PTR] = '\0'; 	//NULL character for UART library
		TxSTRING(TX_BUFFER);
		TX_PTR = 0;
	}
}

/*************************************************************************************
 * Byte transmission function (#device dependent)
 * Transmits a provided character
 ************************************************************************************/
void UART::TxBYTE(char byte)
{	//UCSR0B	&= ~RXIE;	//Disable Rx interrupts while sending data
	while (!( *(module.SR) & USART_SR_TC))
		;
	*module.DR = (byte & 0xFF);
	//UCSR0B	|= RXIE;	//Enable Rx interrupts again after transmission
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









