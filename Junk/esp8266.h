	/*
 * ESP8266.h
 *
 *  Created on: May 29, 2022
 *      Author: antiq
 */

#ifndef ESP8266_H_
#define ESP8266_H_

#include	"GetSetGoConfig.h"
//#include 	"ESP_AT.h"

#define ESP_TASK_PRIORITY 	10
#define ESP_TASK_STACK_SIZE	KB_to_B(2)

	typedef enum{
		DRQ_FLAG_TX = (1<<0)
	}esp_drq_flags_t;

	typedef enum{
		CRQ_FLAG_TX 		= (1<<0),
		CRQ_FLAG_RX 		= (1<<1),
		CRQ_FLAG_PROXY		= (1<<2)
	}esp_crq_flags_t;

    typedef enum{
        ESP_MODE_STA,
        ESP_MODE_AP,
        ESP_MODE_AP_STA
    } esp_mode_t;

	extern unsigned char Echo;
	extern unsigned char PrintVars;

	void ESP_Init(void);
	void TCP_Init(void);
	void UDP_Init(void);
	char ESP_GetData(char *string);
	void tESP_Rx_Engine(void *pvParameters);

#endif /* ESP8266_H_ */
