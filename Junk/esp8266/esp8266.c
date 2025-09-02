
#include "esp8266.h"
#include "esp8266Int.h"

static TaskHandle_t 		espTaskHandle;
static esp_data_t           ESP;
static serialPort_t         *espSerialPort;    
static serial_tx_fn_t       serialtxFn;

// Function Prototypes
static void espTaskHandler(void *pvParameters);

void ESP_Init(serialPort_t  *serialPort)
{
    memset(ESP.ssid,0x00,sizeof(ESP.ssid));
	memset(ESP.password,0x00,sizeof(ESP.password));

	//Network Layer Parameters
	ESP.state 		= ESP_INIT;

    // Manual 
	// strcpy(ESP.SSID,"\"Kapil dev 1960\"");
	// strcpy(ESP.PassWord,"\"antique1960\"");
	// strcpy(ESP.HostName,"\"WiFiWarrior\"");
	// strcpy(ESP.Self_IP,"000.000.000.000");
	// strcpy(ESP.Gtw_IP,"000.000.000.000");
	// strcpy(ESP.Net_Mask,"000.000.000.000");

    // copy the serial port information
    espSerialPort = serialPort;
    SER_openPort(espSerialPort, NULL, 0, serialtxFn, ESP_SERIAL_RX_BUFF_SIZE);
    xTaskCreate(espTaskHandler,"ESP",ESP_TASK_STACK_SIZE, NULL, ESP_TASK_PRIORITY,&espTaskHandle);
}

gsg_result_t ESP_registerSerialTxFn(serial_tx_fn_t txFn)
{
   DEBUG_ASSERT(txFn != NULL);
   serialtxFn = txFn;
   return GSG_SUCCESS;
}


static void espTaskHandler(void *pvParameters)
{
    char hex[100];

    for(;;)
	{
		static uint8_t prvState = 0xFF;
		if(prvState != ESP.state)
		{
			sprintf(hex, "Stat>%d", ESP.state); DEBUG_LOGI(DEBUG_TAG_ESP,"ESP",hex);
			prvState = ESP.state;
		}

		switch(ESP.state)
		{
			case ESP_INIT:
			{
				break;
			}
			
			case ESP_RESET:
			{
				break;
			}
			default:
				break;
		}

        vTaskDelay(pdMS_TO_TICKS(100));
    }
}
