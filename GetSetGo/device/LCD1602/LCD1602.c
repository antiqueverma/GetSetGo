


/********************************************************************
 * LCD1602.c													   		*
 * A platform independent Embedded-C driver for LCD1602        		*
 * The driver has been tested on the below platforms				*
 * 		1. Atmega328p (using AtmelStudio7)							*
 * 		2. MSP430FR2355 (using Code Composer Studio)				*
 * 		3. STM32F407VET6	(using STMCube IDE)						*
 * 																	*
 * Source datasheet link:											*
 * https://www.sparkfun.com/datasheets/LCD/HD44780.pdf         		*
 *																	*
 * 			 					                               		*
 *  Author	: Manish Verma                                      	*
 *  E-mail	: manishv3898@gmail.com , antiQueverma@gmail.com		*
 *  LinkedIn:https://www.linkedin.com/in/manish-verma-551238154		*
 ********************************************************************/
#include "LCD1602.h"

/********************************************************************************************
 * LCD_CONFIG variable stores the configuration of the LCD
 * 							*
 * -----------------------------------------------------------------------------------------*
 * Bits:		|  15   -   12  |   11   -   8  |  7 - 3  |		2    |     1    |    0    |	*
 * LCD_CONFIG = |     LCD_RW    |     LCD_CL    |---------|LCD_STMDEV|  LCD_NP  | LCD_BM  |	*
 *   			|Number of Rows | Number of col.|---------|0 = NonSTM| 0=LOWER 	| 0= 4BIT |	*
 *  			|	   0-15     |      0-15     |---------|1 = STM   | 1=HIGHER | 1= 8BIT |	*
 * -----------------------------------------------------------------------------------------*
 *
 ********************************************************************************************/
unsigned int LCD_CONFIG=0x00;

//Function definitions start from here

/**********************************
*LCD initialization function      *
*Accepts configuration parameters *
***********************************/
void LCD_INIT(int LCD_stm_dev, int LCD_BITMODE, int LCD_NIBPOS, int LCD_ROW, int LCD_COLUMN)
{
	int lcd_port=0,lcd_shift=0;
	if (LCD_NIBPOS)
	{
		lcd_port=0xF0;
		lcd_shift=4;
	}
	else
	{
		lcd_port=0x0F;
		lcd_shift=0;
	}

	//LCD_BITMODE=1:8bit mode,LCD_BITMODE = 0:4bit mode
	//LCD_NIBPOS=0: Lower Nibble of Port, LCD_NIBPOS=1: Higher Nibble of Port
	LCD_CONFIG = (((LCD_COLUMN-1) << 8) & LCD_CL) | (((LCD_ROW-1) << 12) & LCD_RW) | (LCD_BITMODE & LCD_BM)|((LCD_NIBPOS << 1) & LCD_NP) | (LCD_stm_dev << 2)  ;



	if(LCD_CONFIG & LCD_STMDEV)
	{
		LCD_CDIR	|= LCD_bpshifter(LCD_RSPIN|LCD_RWPIN|LCD_ENPIN, LCD_STM32OUT);		//Control pins in output mode
		LCD_DDIR	|= LCD_bpshifter((0xFF*LCD_BITMODE)|lcd_port, LCD_STM32OUT) ;
	}	//Data pins in output mode
	else
	{
		LCD_CDIR	|= LCD_RSPIN | LCD_RWPIN | LCD_ENPIN;
		LCD_DDIR	|= (0xFF*LCD_BITMODE) | (lcd_port);
	}

	LCD_DELAYMS(50);	//As per datasheet


	LCD_cmdWrite();
	LCD_DISABLE;
	LCD_DELAYUS(1);
	if (LCD_CONFIG & LCD_BM)
	{
		//As per data sheet of Hitachi, 8bit instruction is repeated multiple times as a part of initialization
		LCD_DPORT	= BIT8MODEINIT;
		LCD_DELAYMS(5);  //8bit mode
		LCD_epulse();
		LCD_DPORT	= BIT8MODEINIT;
		LCD_DELAYMS(2);  //Second try 8bit mode
		LCD_epulse();
		LCD_DPORT	= BIT8MODEINIT;
		LCD_DELAYMS(1);  //Third try 8bit mode
		LCD_epulse();
		LCD_command(BIT8MODE);  //Set 8bit mode 0b0000111000
	}
	else
	{
		LCD_DPORT	&= ~lcd_port;			//Blanking the data nibble of port
		LCD_DPORT	|= (lcd_port & (BIT4MODEINIT << lcd_shift));
		LCD_DELAYMS(5);  //4bit mode
		LCD_epulse();
		LCD_DPORT	&= ~lcd_port;			//Blanking the data nibble of port
		LCD_DPORT	|= (lcd_port & (BIT4MODEINIT << lcd_shift));
		LCD_DELAYMS(2);  //4bit mode
		LCD_epulse();
		LCD_DPORT	&= ~lcd_port;			//Blanking the data nibble of port
		LCD_DPORT	|= (lcd_port & (BIT4MODEINIT << lcd_shift));
		LCD_DELAYMS(1);  //4bit mode
		LCD_epulse();
		LCD_DPORT	&= ~lcd_port;			//Blanking the data nibble of port
		LCD_DPORT	|= (lcd_port & (0x02 << lcd_shift));
		LCD_DELAYMS(1);  //8bit mode
		LCD_epulse();
		LCD_command(BIT4MODE);  //Set 4bit mode
	}
	LCD_command(LCDCLEAR);
	LCD_command(INCCR);					//Set I/D and S
	LCD_command(CBLNKOFF);
	LCD_command(RETURNHOME);
	LCD_DELAYMS(2);
}

/***************************************
*LCD command function                  *
*Send any required command in CMD mode *
****************************************/
void LCD_command(char lcd_cmd)
{
	if(LCD_CONFIG & LCD_BM){
		//while (LCD_8bit_busy());
		LCD_DELAYMS(1);
		LCD_cmdWrite();
		LCD_8bit_bus(lcd_cmd);}
	else{
		//while (LCD_4bit_busy());
		LCD_DELAYMS(1);
		LCD_cmdWrite();
		LCD_4bit_bus(lcd_cmd);}
}

/***********************************************
 *LCD data function                            *
 *Send provided characters to LCD in data mode *
 ***********************************************/
void LCD_DATA(char x)
{
	if(LCD_CONFIG & LCD_BM){
		//while (LCD_8bit_busy());
		LCD_DELAYMS(1);
		LCD_datMode();
		LCD_8bit_bus(x);}
	else{
		//while (LCD_4bit_busy());
		LCD_DELAYMS(1);
		LCD_datMode();
		LCD_4bit_bus(x);}
}

/**************************************************
*LCD 8bit bus function                            *
*Sends provided byte (instruction/data) to the LCD*
***************************************************/
void LCD_8bit_bus (char bus_val)
{
	LCD_DPORT	= bus_val;	 LCD_DELAYUS(1);
	LCD_epulse();
}

/*****************************************************************
*LCD 4bit bus function                                           *
*Sends provided byte(instruction/data) to the LCD by splitting it*
*Works as per the provided nibble position and port by the user  *
******************************************************************/
void LCD_4bit_bus (char bus_val)
{	int lnib,unib,port=0;		//Bits of port define the nibble
	if(LCD_CONFIG & LCD_NP)
		{port = 0xF0;	unib = (bus_val & port);		lnib = ((bus_val<<4) & port);}//Higher nibble of port
	else
		{port = 0x0F;	unib = ((bus_val>>4) & port);	lnib = (bus_val & port);}//Lower nibble of port

//Send higher nibble on the port
	LCD_DPORT	&= ~port;
	LCD_DPORT	|=	unib;	LCD_DELAYUS(1);
	LCD_epulse();

//Send lower nibble on the port
	LCD_DPORT	&= ~port;
	LCD_DPORT	|=	lnib;	LCD_DELAYUS(1);
	LCD_epulse();
}

/*****************************************
*LCD busy flag function                  *
*Checks the busy flag in 8 and 4 bit mode*
******************************************/
int LCD_8bit_busy(void)
{LCD_cmdRead();
	if(LCD_CONFIG & LCD_STMDEV)
		LCD_DDIR = LCD_bpshifter(0xFF, LCD_STM32IN);	 //Turning Data pins of whole port into input mode
	else
		LCD_DDIR = 0x00;
	LCD_ENABLE;				LCD_DELAYUS(1);
	if(LCD_DINP & BSYFLG)     //if will be executed when busy flag is 1 (LCD is busy)
		{LCD_DISABLE;
		LCD_DELAYUS(100);
		if(LCD_CONFIG & LCD_STMDEV)
			LCD_DDIR = LCD_bpshifter(0xFF,LCD_STM32OUT);
		else
			LCD_DDIR = 0xFF;
		return 1;//Turning Data pins into output mode
		}
	else                  //else will be executed when busy flag is 0 (LCD is free)
		{LCD_DISABLE;
		LCD_DELAYUS(100);
		if(LCD_CONFIG & LCD_STMDEV)
			LCD_DDIR = LCD_bpshifter(0xFF,LCD_STM32OUT);
		else
			LCD_DDIR = 0xFF;
		return 0; //Turning Data pins into output mode
		}
}

int LCD_4bit_busy(void)
{	int port=0;
	if (LCD_CONFIG & LCD_NP)		//Upper nibble
		port = 0xF0;
	else							//Lower nibble
		port = 0x0F;

LCD_cmdRead();

	//Setting the used nibble bits to input direction
	if(LCD_CONFIG & LCD_STMDEV)
		LCD_DDIR &= (LCD_bpshifter(port,LCD_STM32IN)) | (LCD_bpshifter(~port,3));		//3 generates a pattern of 11 which is converted into bit pair format
	else	//#ERROR chances
		LCD_DDIR &= ~port;		//Four bit input mode for particular nibble of port

	LCD_ENABLE;				LCD_DELAYUS(1);
	if(LCD_DINP & BSYFLG)     //if will be executed when busy flag is 1 (LCD is busy)
	{	LCD_epulse();
		//Reverting the direction of used nibble pins
		if(LCD_CONFIG & LCD_STMDEV)
			LCD_DDIR |= (LCD_bpshifter(port,LCD_STM32OUT)) | (LCD_bpshifter(~port,LCD_STM32IN));
		else
			LCD_DDIR |= port;
		return 1;}
	else                  //else will be executed when busy flag is 0 (LCD is free)
	{	LCD_epulse();
		//Reverting the direction of used nibble pins
		if(LCD_CONFIG & LCD_STMDEV)
			LCD_DDIR |= (LCD_bpshifter(port,LCD_STM32OUT)) | (LCD_bpshifter(~port,LCD_STM32IN));
		else //#ERROR Chances
			LCD_DDIR |= port;
		return 0;}
}

//This function only works for STM32 devices, for other devices, it acts as a bypass
int LCD_bpshifter(int lcd_input, int lcd_pairpatt) // Working on 16 bit
{   int value=0;
	if(LCD_CONFIG & LCD_STMDEV)
		{for(int i=0; i <= 7 ; i++)		//Because LCD has only 8 data pins
			if(lcd_input & (1<<i))
				value |= lcd_pairpatt << (i*2);
		return (value & 0xFFFF);		//0xFFFF because the returned value is 16 bit
		}
	else
		return lcd_input;
}

void LCD_NEWMSG(char lcd_string[])
{
	  LCD_CLEAR();
	  LCD_HOME();
	  LCD_CUROFF();
	  LCD_STRING(lcd_string);
}

void LCD_NEWMSG_L2(char lcd_string[])
{
	  LCD_SETCUR(1,0);
	  LCD_STRING(lcd_string);
}

/*******************************************************************
*LCD data display functions                                        *
*These functions can display most of the data types                *
*Possible types include - integer, float, binary, character, string*
********************************************************************/
void LCD_INTEGER(int num)
{	char digits[6];
	int x,y=num,i=0,d=1,f=0;

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
			if((x != 0) || (f == 1))		//Removal of heading zeros that do not represent any information
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
	digits[i]='\0'; //Manually append a NULL Character
	LCD_STRING(digits);
}

void LCD_STRING (char x[])
{	int i=0;
	while(x[i] != '\0')				//Detect end of string
	{
		LCD_DATA(x[i]);
		LCD_command(INCCR);
		i++;
	}
}

void LCD_FLOAT(float num)
{	char digits[10];
	int x=0,i=0,d=10000,f=0,yi=0,yf=0;
	yi = num;  //This is the integral part
	yf = ((num - (float)yi)*1000.0);	//This is the fractional part
	//Conversion of the integral part into string
	while(d>0)
	{	x=yi/d;
		if((x != 0) || (f == 1))
		{
			f=1;
			digits[i] = x + '0';
			i++;
		}
		yi=yi-x*d;
	d=d/10;}
	//Conversion of the fractional part into string
	digits[i]='.'; //Placing a decimal point
	i++;
	d=100;   //Works only upto 3 decimal places
	while(d>0)
	{
		x=yf/d;
		digits[i] = x + '0';
		i++;
		yf=yf-x*d;
		d=d/10;
	}
	digits[i]='\0';  //Placing a NULL character after the number ends
	LCD_STRING(digits);
}

void LCD_BINARY(int num, int hzero)
{	char bits[17];
 	int  ptr=0;
 	int i=0,flg=0;
 	if(hzero)
 	   flg=1;
 	for(i = 0;i <= 15;i++)
 		{if(num & ( 1 << (15-i)) )
 		    	{bits[ptr] = '1';
 		    	flg=1;
 		    	ptr++;}
 	    else
 	        if (flg)
 		        {bits[ptr] = '0';
 		        ptr++;}
 		}
 	if(ptr == 0)
 	    {bits[ptr] = '0'; ptr++;}
 	bits[ptr] = '\0';
    LCD_STRING(bits);
}


/*****************************************
*LCD display control functions           *
******************************************/
void LCD_CLEAR() //This needs time
{LCD_command(LCDCLEAR); LCD_DELAYMS(2);}

void LCD_HOME() //This needs time
{LCD_command(RETURNHOME); LCD_DELAYMS(2);}

void LCD_INC()
{LCD_command(INCCR);}

void LCD_DEC()
{LCD_command(DECCR);}

void LCD_CURON()
{LCD_command(CDISPON);}

void LCD_CUROFF()
{LCD_command(CDISPOFF);}

void LCD_BLINKON()
{LCD_command(CBLNKON);}

void LCD_BLINKOFF()
{LCD_command(CBLNKOFF);}

void LCD_SHIFTL()
{LCD_command(LSHIFTTEXT);}

void LCD_SHIFTR()
{LCD_command(RSHIFTTEXT);}

void LCD_SETCUR(char row, char col) //Receives max row=1, col=15
{	int c_address=0;
	if (row <= 0)
	c_address = col;
	else
	c_address = 0x40 + col;
	LCD_command(CPOS|c_address);
}

void LCD_FirstLineMsg (char x[])
{	LCD_CLEAR();
	LCD_HOME();
	LCD_STRING(x);
}

void LCD_epulse()
{
		LCD_DISABLE;	//LCD_DELAYUS(1);	//Worked in AVR without this delay
		LCD_ENABLE;		LCD_DELAYUS(1);//	//This delay is important
		LCD_DISABLE;	//LCD_DELAYUS(1);	//Worked in AVR without this delay
}

void LCD_cmdWrite()
{
	LCD_IMODE;	LCD_DELAYUS(1);		//This delay is important
	LCD_WRITE;  LCD_DELAYUS(1);		//This delay is important
}

void LCD_cmdRead()
{
	LCD_IMODE;//
	LCD_READ;//
}

void LCD_datMode()
{
	LCD_DMODE;//
	LCD_WRITE;//
}
