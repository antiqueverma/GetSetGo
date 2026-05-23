
#include "esp8266.h"
#include "esp8266Int.h"
#include "esp8266at.h"

static TaskHandle_t 		espTaskHandle;
static esp_data_t           ESP;
static serial_port_t         *espSerialPort;
static serial_tx_fn_t       serialtxFn;

#define ESP_RX_BUFFER_SIZE	100
static char esp_rx_buff[ESP_RX_BUFFER_SIZE];
static char esp_argv_buff[ESP_RX_BUFFER_SIZE];


#define ESP_TX_BYTE_TO	1
#define ESP_RX_BYTE_TO	1000

// Function Prototypes
static void espTaskHandler(void *pvParameters);
static uint8_t espStateFnInit( uint8_t rcvdResp );
static uint8_t espGetAtResponse(uint8_t statePhase);
static void rxResponseTimeoutEvent(void);
uint16_t atCmdRxParser(char *rxCmd, uint8_t *argc, char *argv);

void ESP_Init(serial_port_t  *serialPort)
{
    memset(ESP.ssid,0x00,sizeof(ESP.ssid));
	memset(ESP.password,0x00,sizeof(ESP.password));

	//Network Layer Parameters
	ESP.state 		= ESP_INIT;

    // Manual 
	strcpy(ESP.ssid,"\"Kapil dev 1960\"");
	strcpy(ESP.password,"\"antique1960\"");
	strcpy(ESP.hostName,"\"WiFiWarrior\"");
	strcpy(ESP.espIP,"000.000.000.000");
	strcpy(ESP.gtwIP,"000.000.000.000");
	strcpy(ESP.subnetMask,"000.000.000.000");

    // copy the serial port information
	DEBUG_ASSERT(serialPort != NULL);
    espSerialPort = serialPort;
	
    xTaskCreate(espTaskHandler,"ESP",ESP_TASK_STACK_SIZE, NULL, ESP_TASK_PRIORITY,&espTaskHandle);
}

//gsg_result_t ESP_registerSerialTxFn(serial_tx_fn_t txFn)
//{
//   DEBUG_ASSERT(txFn != NULL);
//   serialtxFn = txFn;
//   return GSG_SUCCESS;
//}
char hex[200];

static void espTaskHandler(void *pvParameters)
{

	uint8_t statePhase = 0;
	espat_rx_cmd_t rcvdResp = AT_R_UNKWN_CMD;

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
				statePhase = espStateFnInit(rcvdResp);
				break;
			}
			case ESP_CONNECTED_IDLE:
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
		rcvdResp = espGetAtResponse(statePhase);
        vTaskDelay(pdMS_TO_TICKS(100));
    }
	vTaskDelete(NULL);
}

static uint8_t espStateFnInit( uint8_t rcvdResp )
{
	//DEBUG_FN_ENTRY();
	static uint8_t seqCtr = 0x00;
	static uint8_t phaseCtr = 0x00;
	static uint8_t flags = 0;

	sprintf(hex,"Init=%d",seqCtr);	DEBUG_LOGI(DEBUG_TAG_ESP,"ESP",hex);

	switch(seqCtr)
	{
		case 0:
		{
			if(!phaseCtr)
			{
				//Try to disconnect any previously open connections
				vTaskDelay(pdMS_TO_TICKS(1000));
				SER_sendString(espSerialPort, "+++", ESP_TX_BYTE_TO);
				vTaskDelay(pdMS_TO_TICKS(300)); // Guard time after +++ is needed. This is what Espressif says.

				vTaskDelay(pdMS_TO_TICKS(300)); //Make sure the ESP is silent now.
				SER_flushRxBuffer(espSerialPort);
				vTaskDelay(pdMS_TO_TICKS(300));
				SER_flushRxBuffer(espSerialPort);

				SER_sendString(espSerialPort,  ESP_CMD_OUT[AT_T] ,ESP_TX_BYTE_TO);
				SER_sendString(espSerialPort, "\r\n", 0);
				phaseCtr++;
				break;
			}
			else
			{
				if(rcvdResp == AT_R_UNKWN_CMD)
				{
					phaseCtr++;
				}
				else if(rcvdResp == AT_R_OK)
				{
					phaseCtr = 0;
					seqCtr++;	//move to next command
				}
			}
			break;
		}
		case 1:		//Turn OFF command echo
		{
			if(!phaseCtr)
			{
				SER_sendString(espSerialPort,  ESP_CMD_OUT[AT_T_ECHO_OFF] ,ESP_TX_BYTE_TO);
				SER_sendString(espSerialPort, "\r\n", 0);
				phaseCtr++;
			}
			else
			{
				if(rcvdResp == AT_R_UNKWN_CMD)
				{
					phaseCtr++;
				}
				else if(rcvdResp == AT_R_OK)
				{
					phaseCtr = 0;
					seqCtr++;
				}
			}
			break;
		}
		case 2:		//Set Current Mode
		{
			if(!phaseCtr)
			{
				//SER_sendString(espSerialPort,  ESP_CMD_OUT[AT_T_CHK_CUR_MODE] ,ESP_TX_BYTE_TO);
				SER_sendString(espSerialPort,  ESP_CMD_OUT[AT_T_SET_CUR_MODE] ,ESP_TX_BYTE_TO);
				SER_sendString(espSerialPort,  "1" , ESP_TX_BYTE_TO);
				SER_sendString(espSerialPort, "\r\n", 0);
				phaseCtr++;
			}
			else
			{
				if(rcvdResp == AT_R_CUR_MODE)
				{
					phaseCtr++;
				}
				else if(rcvdResp == AT_R_OK)
				{
					phaseCtr = 0;
					seqCtr++;
				}
			}
			break;
		}
		case 3:		//Check Current AP
		{
			if(!phaseCtr)
			{
				flags = 0x00;
				SER_sendString(espSerialPort,  ESP_CMD_OUT[AT_T_CHK_CUR_AP] ,ESP_TX_BYTE_TO);
				SER_sendString(espSerialPort, "\r\n", 0);
				phaseCtr++;
			}
			else
			{
				if(rcvdResp == AT_R_NO_AP)
				{
					SET_BIT_(flags,0);
				}
				else if(rcvdResp == AT_R_CUR_AP)
				{
					SET_BIT_(flags,1);
				}
				else if(rcvdResp == AT_R_OK)
				{
					if(GET_BIT_(flags,0))
						seqCtr = 5;	//jump to connect AP, if it is already disconnected
					else if(GET_BIT_(flags,0))
						seqCtr = 6;  //need to check here if it is connected to the preferred ap

					phaseCtr = 0;
					flags = 0x00;
				}
			}
			break;
		}
		case 4:		//Disconnect from current AP
		{
			if(!phaseCtr)
			{
				SER_sendString(espSerialPort,  ESP_CMD_OUT[AT_T_DISCON_AP] ,ESP_TX_BYTE_TO);
				SER_sendString(espSerialPort, "\r\n", 0);
				phaseCtr++;
			}
			else
			{
				if(rcvdResp == AT_R_OK)
				{
					phaseCtr++;
				}
				else if((rcvdResp == AT_R_WIFI_DISCON) || (rcvdResp == AT_R_Q_EMPTY))
				{
					phaseCtr = 0;
					seqCtr++;
				}
			}
			break;
		}
		case 5:		//Connect to AP
		{
			if(!phaseCtr)
			{
				SER_sendString(espSerialPort,  ESP_CMD_OUT[AT_T_CONNECT_2_AP_CUR] ,ESP_TX_BYTE_TO);
				SER_sendString(espSerialPort,  ESP.ssid ,ESP_TX_BYTE_TO);
				SER_sendString(espSerialPort,  "," ,ESP_TX_BYTE_TO);
				SER_sendString(espSerialPort,  ESP.password ,ESP_TX_BYTE_TO);
				SER_sendString(espSerialPort, "\r\n", 0);

				vTaskDelay(pdMS_TO_TICKS(1000));
				phaseCtr++;
			}
			else
			{
				if((rcvdResp == AT_R_WIFI_CONN) || (rcvdResp == AT_R_GOT_IP))
				{
					phaseCtr++;
				}
				else if(rcvdResp == AT_R_OK)
				{
					phaseCtr = 0;
					seqCtr++;
				}
				else if(rcvdResp == AT_R_BUSY)
				{
					vTaskDelay(pdMS_TO_TICKS(1000));
				}
			};
			break;
		}
		case 6:		//Check Current IP
		{
			if(!phaseCtr)
			{	flags = 0x00;
				SER_sendString(espSerialPort,  ESP_CMD_OUT[AT_T_CHK_CUR_IP] ,ESP_TX_BYTE_TO);
				SER_sendString(espSerialPort, "\r\n", 0);
				phaseCtr++;
			}
			else
			{
				if(rcvdResp == AT_R_STA_CUR_IP)
				{
					SET_BIT_(flags,0);
					strcpy(ESP.espIP,esp_argv_buff);
				}
				else if (rcvdResp == AT_R_STA_CUR_GTW)
				{
					SET_BIT_(flags,1);
					strcpy(ESP.gtwIP,esp_argv_buff);
				}
				else if(rcvdResp == AT_R_STA_CUR_NET_MSK)
				{
					SET_BIT_(flags,2);
					strcpy(ESP.subnetMask,esp_argv_buff);
				}
				else if(rcvdResp == AT_R_OK)
				{
					sprintf(hex,"\n%s",ESP.espIP);	DEBUG_LOGI(DEBUG_TAG_ESP,"ESP",hex);
					sprintf(hex,"--%s",ESP.gtwIP);		DEBUG_LOGI(DEBUG_TAG_ESP,"ESP",hex);
					sprintf(hex,"--%s",ESP.subnetMask);	DEBUG_LOGI(DEBUG_TAG_ESP,"ESP",hex);
					phaseCtr = 0;
					seqCtr++;
				}
			}
			break;
		}
		case 7:		//Delete any existing TCP Server
		{
			if(!phaseCtr)
			{
				SER_sendString(espSerialPort,  ESP_CMD_OUT[AT_T_CONFIG_TCP_SERVER] ,ESP_TX_BYTE_TO);
				SER_sendString(espSerialPort,  "0" ,ESP_TX_BYTE_TO);
				SER_sendString(espSerialPort, "\r\n", 0);
				phaseCtr++;
			}
			else
			{
				if(rcvdResp == AT_R_NO_CHANGE)
				{
					phaseCtr++;
				}
				if((rcvdResp == AT_R_OK) || (rcvdResp == AT_R_ERR))		//Error received here means there was no TCP server to delete
				{
					phaseCtr = 0;
					seqCtr++;
				}
			}
			break;
		}
		case 8:
		{
			if(!phaseCtr)
			{
				SER_sendString(espSerialPort,  ESP_CMD_OUT[AT_T_SET_MULTI_CONNECT] ,ESP_TX_BYTE_TO);
				SER_sendString(espSerialPort,  "0" ,ESP_TX_BYTE_TO);  //###
				SER_sendString(espSerialPort, "\r\n", 0);
				phaseCtr++;
			}
			else
			{
				if(rcvdResp == AT_R_OK)
				{
					phaseCtr = 0;
					seqCtr++;
				}
			}
			break;
		}
		case 9:
		{
			if(!phaseCtr)
			{
				SER_sendString(espSerialPort,  ESP_CMD_OUT[AT_T_SET_ADD_PORT_RX_PKT] ,ESP_TX_BYTE_TO);
				SER_sendString(espSerialPort,  "1" ,ESP_TX_BYTE_TO);
				SER_sendString(espSerialPort, "\r\n", 0);
				phaseCtr++;
			}
			else
			{
				if(rcvdResp == AT_R_OK)
				{
					phaseCtr = 0;
					seqCtr++;
				}
			}
			break;
		}
		default:
		{
			break;
		}
		/*case 9:			//List available APs
		{
				SER_sendString(espSerialPort,  ESP_CMD_OUT[AT_T_LIST_AVLB_AP] ,ESP_TX_BYTE_TO);
				SER_sendString(espSerialPort,  "\"R&D-System\"" ,ESP_TX_BYTE_TO);//AT_T_SET_HOSTNAME
				SER_sendString(espSerialPort, "\r\n", 0);
			break;
		}*/
	}
	if(seqCtr > 9)
	{
		ESP.state = ESP_CONNECTED_IDLE;
		// ESP_rqstConnection(NULL, CONN_UDP_PROXY, "192.168.1.40", 6000, 0, 0, CRQ_FLAG_PROXY);
		phaseCtr = 0;
	}

	return phaseCtr;
}


static uint8_t espGetAtResponse(uint8_t statePhase)
{
	//DEBUG_FN_ENTRY();

	uint8_t buffPtr = 0;
	uint8_t detCmd = AT_R_UNKWN_CMD;		//index of detected command
	uint8_t argc = 0;
	if(statePhase)  //Wait for a response after sending a command
	{
		uint8_t rxData = 0;
		memset(esp_rx_buff,0x00,sizeof(esp_rx_buff));	//esp_rx_buff[ESP_RX_BUFFER_SIZE] is a static buffer for this file
		DEBUG_LOGI(DEBUG_TAG_ESP,"ESP","#A");
		//Receive all bytes until the Serial Queue is empty
		while(SER_receiveData(espSerialPort, &rxData, 1, ESP_RX_BYTE_TO))
		{	DEBUG_LOGI(DEBUG_TAG_ESP,"ESP","#B");
			esp_rx_buff[buffPtr] = rxData;
			buffPtr++;
			if (rxData == '\r' || rxData == '\n' || rxData == '>')
			{
				// DEBUG_LOGI(DEBUG_TAG_ESP,"ESP",esp_rx_buff);

				if((buffPtr == 1) && (rxData != '>'))  // blank \r\n were received
				{
					memset(esp_rx_buff,0x00,sizeof(esp_rx_buff));
					buffPtr = 0;
					rxData = 0;
					continue;
				}

				detCmd = atCmdRxParser(esp_rx_buff, &argc, esp_argv_buff);

				if(detCmd == AT_R_RX_DATA)
				{
					;
				}
				// sprintf(hex,"Cm:%d",detCmd); DEBUG_LOGI(DEBUG_TAG_ESP,"ESP",hex);
				if(detCmd == AT_R_UNKWN_CMD)
				{
					snprintf(hex, sizeof(hex), "Cmd:%s", esp_rx_buff);DEBUG_LOGI(DEBUG_TAG_ESP,"ESP",hex);
				}
				break;	//Do not retrieve the next command until this one is processed by the task
			}
		}

		if(buffPtr == 0)	//queue was empty
		{
			rxResponseTimeoutEvent();
			return AT_R_Q_EMPTY;
		}
	}
	else
	{
		detCmd = AT_R_UNKWN_CMD;
	}

	return detCmd;
}

static void rxResponseTimeoutEvent(void)
{
	;//Implement later
}

uint16_t atCmdRxParser(char *rxCmd, uint8_t *argc, char *argv)
{
    // Step 1: Match the rxCmd with entries in ESP_CMD_IN
    for (uint16_t i = 0; i < __AT_R_LASTCMD; i++)
    {
        size_t cmdLen = strlen(ESP_CMD_IN[i]);
        if (strncmp(rxCmd, ESP_CMD_IN[i], cmdLen) == 0)
        {
            // Command matched
            rxCmd += cmdLen; // Move pointer past the matched command
            *argc = 0;       // Initialize argument count

            // Step 2: Parse arguments (separated by ':' or ',')
            char *argPtr = argv;
            while (*rxCmd)
            {
                if (*rxCmd == ':' || *rxCmd == ',')
                {
                    rxCmd++; // Skip the separator
                    continue;
                }

                // Handle string arguments with double quotes
                if (*rxCmd == '"')
                {
                    rxCmd++; // Skip the leading double quote
                    while (*rxCmd && *rxCmd != '"') // Copy until the closing quote
                    {
                        *argPtr++ = *rxCmd++;
                    }
                    if (*rxCmd == '"')
                    {
                        rxCmd++; // Skip the trailing double quote
                    }
                }
                else
                {
                    // Copy non-quoted arguments
                    while (*rxCmd && *rxCmd != ':' && *rxCmd != ',' && *rxCmd != '\0')
                    {
                        *argPtr++ = *rxCmd++;
                    }
                }

                *argPtr++ = '\0'; // Null-terminate each argument
                (*argc)++;        // Increment argument count
            }
            return i; // Return index of the matched command
        }
    }

    // Step 3: If no match found, return 0xFFFF
    return 0xFF;
}
