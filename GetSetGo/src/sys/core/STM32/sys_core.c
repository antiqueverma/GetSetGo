#include "SysCore.h"

/*-----System Core Clock Configuration using RCC module -----*/
void SystemClockSetup(void)
{
	#define RCC_CFGR_PLLM_4 		4
	#define RCC_CFGR_PLLN_168  		168
	#define RCC_CFGR_PLLP_2 		0  // PLLP = 2

	/*************STEPS FOLLOWED*************************************************
	 * 1. ENABLE HSE and wait for the HSE to become Ready
	 * 2. Set the POWER ENABLE CLOCK and VOLTAGE REGULATOR
	 * 3. Configure the FLASH PREFETCH and the LATENCY Related Settings
	 * 4. Configure the PRESCALARS HCLK, PCLK1, PCLK2
	 * 5. Configure the MAIN PLL
	 * 6. Enable the PLL and wait for it to become ready
	 * 7. Select the Clock Source and wait for it to be set
	 * **************************************************************************/
	//Enable 8MHz External Crystal and for it to get ready
	RCC->CR |= RCC_CR_HSEON;
	while (!(RCC->CR & RCC_CR_HSERDY)) ;

	//Set the POWER ENABLE CLOCK and VOLTAGE REGULATOR
	RCC->APB1ENR |= RCC_APB1ENR_PWREN;
	PWR->CR |= PWR_CR_VOS;	//No scaling

	//Configure the FLASH PREFETCH and the LATENCY Related Settings
	FLASH->ACR = FLASH_ACR_ICEN | FLASH_ACR_DCEN | FLASH_ACR_PRFTEN | FLASH_ACR_LATENCY_5WS;		//This should be 5

	//Configure the PRESCALARS HCLK, PCLK1, PCLK2
	RCC->CFGR |= RCC_CFGR_HPRE_DIV1 | RCC_CFGR_PPRE1_DIV4 | RCC_CFGR_PPRE2_DIV2;

	//Configure the MAIN PLL
	RCC->PLLCFGR = (RCC_CFGR_PLLM_4 << RCC_PLLCFGR_PLLM_Pos) | (RCC_CFGR_PLLN_168 << RCC_PLLCFGR_PLLN_Pos) | (RCC_CFGR_PLLP_2 << RCC_PLLCFGR_PLLP_Pos) | RCC_PLLCFGR_PLLSRC_HSE;

	//Enable the PLL and wait for it to become ready
	RCC->CR |= RCC_CR_PLLON;
	while(!(RCC->CR & RCC_CR_PLLRDY));

	//Select the Clock Source and wait for it to be set
	RCC->CFGR |= RCC_CFGR_SW_PLL;
	while ((RCC->CFGR & RCC_CFGR_SWS) != RCC_CFGR_SWS_PLL);

	RCC->APB2ENR |= (RCC_APB2ENR_USART1EN);

	/*TIMER6 connected to APB1 is used for delays.
	It is started once and keeps running.
	Whenever a delay is needed, the COUNTER is cleared and the COUNT value is compared.*/
	RCC->APB1ENR |= (RCC_APB1ENR_TIM6EN);	//Enable clock of TIMER6 for delays
	TIM6->PSC = 83;	//Prescalar = 84-1 for APB1 = 84MHz
	TIM6->ARR = 0xFFFF;	//Maximum value upto which Counter can count
	TIM6->CR1 |= TIM_CR1_CEN;
	while(!(TIM6->SR & TIM_SR_UIF));	//UIF: Update interrupt flag. This bit is set by the hardware when the registers are updated
}

//This function uses TIMER6 for producing delay in uSeconds
void usDelay(unsigned int Tus)
{
	TIM6->CNT = 0x00;
	while(TIM6->CNT < Tus);
	//TIM6->CR1 &= ~TIM_CR1_CEN;
}

//This function uses usDelay for producing delay in mSeconds
void msDelay(unsigned int Tms)
{
	for(unsigned int i=0; i < Tms ; i++)
		usDelay(1000);
}
