/*
 * ESP8266.c
 *
 *  Created on: May 29, 2022
 *      Author: antiq
 */

#include "esp8266.h"
#include "esp8266Int.h"

//Generic variables
#define FILE_PREFIX "ESP"
TaskHandle_t 		espTaskHandle; // New field to store Tx task handle
static uint8_t moduleInit = 0;
static void espTaskHandler(void *pvParameters);
//static char hex[100];

#define PROXY_TX_QUEUE_LENGTH	100
#define ESP_TASKS_QUEUE_LENGTH	5
#define DAT_RQST_QUEUE_LENGTH	5
#define ESP_EN_RQST_PRIORITY 	0

static QueueHandle_t 		conRqstQueueHandle;
static QueueHandle_t 		datRqstQueueHandle;
static QueueHandle_t		proxyTxQueueHandle;
static SemaphoreHandle_t 	pendingRqstSemaphore;
uint8_t serPort;


//Timeout Values
#define SER_LOAD_TX_TO				pdMS_TO_TICKS(1)
#define SER_GET_RX_TO				pdMS_TO_TICKS(1)
#define RQST_QUEUE_TO 				0 // Timeout for queue wait (500ms or as defined)
#define DEFAULT_TX_QUEUE_LENGTH		100
#define DEFAULT_RX_QUEUE_LENGTH		100


#define SER_loadTxTerminate()		SER_loadTxString(serPort, "\r\n" ,0, SER_LOAD_TX_TO)


//Function Prototypes
static void rxResponseTimeoutEvent(void);

//State machine Functions
static uint8_t espStateFnInit( uint8_t rcvdResp );
static uint8_t espStateFnStartUDPClient( uint8_t rcvdResp, esp_con_rqst_t *connRqst);
static uint8_t espStateFnOpenUDPProxy( uint8_t rcvdResp, esp_con_rqst_t *connRqst);
static uint8_t espStateFnCloseUDPProxy( uint8_t rcvdResp, esp_con_rqst_t *connRqst);
static uint8_t espStateFnTxInProgress( uint8_t rcvdResp, esp_dat_rqst_t *datRqst );
static uint8_t espStateFnUDPProxy( uint8_t rcvdResp);

// Other Functions
result_code_t espDatRqstHandler( esp_dat_rqst_t *datRqst );
result_code_t espConRqstHandler( esp_con_rqst_t *connRqst );
result_code_t espGetConRqstFromQueue(esp_con_rqst_t *connRqst);
result_code_t espGetDatRqstFromQueue(esp_dat_rqst_t *datRqst);
static void espCloseProxyChannels(void);

static uint8_t espGetFreeLinkId( void );
static uint8_t espGetAtResponse(uint8_t statePhase);
result_code_t espConRqstHandler( esp_con_rqst_t *connRqst );
result_code_t espChkIPAddValidity(char *ipAddress);
result_code_t espGetRqstFromQueue(esp_con_rqst_t *connRqst);
static result_code_t espRegisterSocket(esp_con_rqst_t *connRqst);

//File specific variables
#define		MAX_SUPPORTED_CONNECTIONS	4

#define ESP_RX_BUFFER_SIZE	100
char esp_rx_buff[ESP_RX_BUFFER_SIZE];
char esp_argv_buff[ESP_RX_BUFFER_SIZE];
//Imported variables



//Private variables
static esp_data_t 		ESP;
static esp_socket_t		espConnections[MAX_SUPPORTED_CONNECTIONS];  // Established connections

void ESP_Init(void)
{
	serPort = 0xFF;
	memset(ESP.SSID,0x00,sizeof(ESP.SSID));
	memset(ESP.PassWord,0x00,sizeof(ESP.PassWord));

	//Network Layer Parameters
	ESP.Mode		= 0x00;
	ESP.Conn_Count 	= 0;
	ESP.State 		= ESP_INIT;
	strcpy(ESP.SSID,"\"Kapil dev 1960\"");
	strcpy(ESP.PassWord,"\"antique1960\"");
	strcpy(ESP.HostName,"\"WiFiWarrior\"");
	strcpy(ESP.Self_IP,"000.000.000.000");
	strcpy(ESP.Gtw_IP,"000.000.000.000");
	strcpy(ESP.Net_Mask,"000.000.000.000");

	uint8_t i=0;
	for(i=0 ; i<MAX_SUPPORTED_CONNECTIONS ; i++)
	{
		memset(espConnections[i].targetIP, 0xFF , sizeof(espConnections[i].targetIP));
		espConnections[i].ConnType = __CONN_TYPE_MAX;
		espConnections[i].connTO	= UINT16_MAX;
		espConnections[i].linkId	= i;
		espConnections[i].port		= UINT16_MAX;
		espConnections[i].flags		= 0;
		espConnections[i].connStat	= CONN_STAT_UNFORMED;
		espConnections[i].txQueueLength = 0;
		espConnections[i].rxQueueLength = 0;
	}

	conRqstQueueHandle =  xQueueCreate(ESP_TASKS_QUEUE_LENGTH, sizeof(esp_con_rqst_t));
	datRqstQueueHandle =  xQueueCreate(DAT_RQST_QUEUE_LENGTH, sizeof(esp_dat_rqst_t));
	proxyTxQueueHandle =  xQueueCreate(PROXY_TX_QUEUE_LENGTH, sizeof(uint8_t));

	pendingRqstSemaphore = xSemaphoreCreateCounting( CON_RQST_QUEUE_LENGTH + DAT_RQST_QUEUE_LENGTH + 1, 0);
	moduleInit = 0;

	xTaskCreate(espTaskHandler,"ESP",ESP_TASK_STACK_SIZE,NULL,ESP_TASK_PRIORITY,&espTaskHandle);
}

static void espTaskHandler(void *pvParameters)
{
	serPort = SER_getSerialInstance(espTaskHandle,0,1,&huart1);
	if(serPort == 0xFF)
	{
		sprintf(hex,"SpAcqFail");DEBUG_sendHex2Ch(3);
		vTaskSuspend(NULL);
	}
	//serPort = 0;
	TickType_t xLastWakeTime;
	uint8_t rcvdResp = AT_R_UNKWN_CMD;

	//UBaseType_t uxHighWaterMark;	uxHighWaterMark = uxTaskGetStackHighWaterMark( NULL );

	while(1)
	{
		static esp_con_rqst_t connRqst;
		static esp_dat_rqst_t datRqst;

		static uint8_t statePhase = 0;
		vTaskDelay(pdMS_TO_TICKS(100));

		//DEBUG_FN_ENTRY();

		static uint8_t prvState = 0xFF;
		if(prvState != ESP.State)
		{
			sprintf(hex, "Stat>%d", ESP.State); DEBUG_sendHex3;
			prvState = ESP.State;
		}

		switch(ESP.State)
		{
			case ESP_INIT:
			{
				statePhase = espStateFnInit(rcvdResp);
				break;
			}
			case ESP_CONNECTED_IDLE:
			case ESP_IDLE:
			{
				result_code_t result = RC_FAILURE;

				xSemaphoreTake(pendingRqstSemaphore, pdMS_TO_TICKS(500));

				// Check for connection requests
				result = espGetConRqstFromQueue(&connRqst);
				if (result == RC_SUCCESS)
				{
					result = espConRqstHandler(&connRqst);
					// Notify the requesting task of result code  if (connRqst.task != NULL)	xTaskNotify(connRqst.task, result, eSetValueWithOverwrite);
				}
				else// Check for data requests
				{
					result = espGetDatRqstFromQueue(&datRqst);
					if (result == RC_SUCCESS)
					{
						result = espDatRqstHandler(&datRqst);
						// Notify the requesting task of result code	if (datRqst.task != NULL)		xTaskNotify(datRqst.task, result, eSetValueWithOverwrite);
					}
				}

				break;
			}
			case ESP_START_TCP_SERVER:
			{
				ESP.State = ESP_CONNECTION_ESTD;
				break;
			}
			case ESP_START_TCP_CLIENT:
			{
				ESP.State = ESP_CONNECTION_ESTD;
				break;
			}
			case ESP_START_UDP_CLIENT:
			{
				statePhase = espStateFnStartUDPClient(rcvdResp, &connRqst);
				break;
			}
			case ESP_CONNECTION_ESTD:
			{
				//Register the socket
				espRegisterSocket(&connRqst);


				ESP.State = ESP_CONNECTED_IDLE;
				break;
			}
			case ESP_TXIP:
			{
				statePhase = espStateFnTxInProgress(rcvdResp, &datRqst);
				break;
			}
			case ESP_OPEN_UDP_PROXY:
			{
				statePhase = espStateFnOpenUDPProxy(rcvdResp, &connRqst);
				moduleInit = 1;
				//espRegisterSocket(&connRqst);
				break;
			}
			case ESP_UDP_PROXY:
			{
				statePhase = espStateFnUDPProxy(rcvdResp);
				break;
			}
			case ESP_CLOSE_UDP_PROXY:
			{
				statePhase = espStateFnCloseUDPProxy(rcvdResp, &connRqst);
				break;
			}
			case ESP_RESET:
			{
				break;
			}
			default:
				break;
		}

		//vTaskDelay(pdMS_TO_TICKS(10));
		rcvdResp = espGetAtResponse(statePhase);

		//sprintf(hex,"Stk:%u",(uint16_t)uxHighWaterMark); DEBUG_sendHex3;	uxHighWaterMark = uxTaskGetStackHighWaterMark( NULL );
	}

	vTaskDelete(NULL);
}




//Esp State Machine functions
#if  1

static uint8_t espStateFnInit( uint8_t rcvdResp )
{
	//DEBUG_FN_ENTRY();
	static uint8_t seqCtr=0x00;
	static uint8_t phaseCtr=0x00;
	static uint8_t flags=0;

	//sprintf(hex,"Init=%d",seqCtr);	DEBUG_sendHex2Ch(3);

	switch(seqCtr)
	{
		case 0:
		{
			if(!phaseCtr)
			{
				//Try to disconnect any previously open connections
				vTaskDelay(pdMS_TO_TICKS(1000));
				SER_loadTxString(serPort,"+++",0, 1);
				vTaskDelay(pdMS_TO_TICKS(1000)); // Guard time after +++ is needed. This is what Espressif says.

				//Make sure the ESP is silent now.
				vTaskDelay(pdMS_TO_TICKS(300));
				SER_flushAllRxData(serPort);
				vTaskDelay(pdMS_TO_TICKS(300));
				SER_flushAllRxData(serPort);

				SER_loadTxString(serPort, CMD_OUT[AT_T] ,0, SER_LOAD_TX_TO);
				SER_loadTxTerminate();
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
				SER_loadTxString(serPort, CMD_OUT[AT_T_ECHO_OFF] ,0, SER_LOAD_TX_TO);
				SER_loadTxTerminate();
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
				//SER_loadTxString(serPort, CMD_OUT[AT_T_CHK_CUR_MODE] ,0, SER_LOAD_TX_TO);
				SER_loadTxString(serPort, CMD_OUT[AT_T_SET_CUR_MODE] ,0, SER_LOAD_TX_TO);
				SER_loadTxString(serPort, "1" ,1, SER_LOAD_TX_TO);
				SER_loadTxTerminate();
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
				SER_loadTxString(serPort, CMD_OUT[AT_T_CHK_CUR_AP] ,0, SER_LOAD_TX_TO);
				SER_loadTxTerminate();
				phaseCtr++;
			}
			else
			{
				if(rcvdResp == AT_R_NO_AP)
				{
					flags |= BIT0;
				}
				else if(rcvdResp == AT_R_CUR_AP)
				{
					flags |= BIT1;
				}
				else if(rcvdResp == AT_R_OK)
				{
					if(flags & BIT0)
						seqCtr = 5;	//jump to connect AP, if it is already disconnected
					else if(flags & BIT1)
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
				SER_loadTxString(serPort, CMD_OUT[AT_T_DISCON_AP] ,0, SER_LOAD_TX_TO);
				SER_loadTxTerminate();
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
				SER_loadTxString(serPort, CMD_OUT[AT_T_CONNECT_2_AP_CUR] ,0, SER_LOAD_TX_TO);
				SER_loadTxString(serPort, ESP.SSID ,0, SER_LOAD_TX_TO);
				SER_loadTxString(serPort, "," ,0, SER_LOAD_TX_TO);
				SER_loadTxString(serPort, ESP.PassWord ,0, SER_LOAD_TX_TO);
				SER_loadTxTerminate();

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
				SER_loadTxString(serPort, CMD_OUT[AT_T_CHK_CUR_IP] ,0, SER_LOAD_TX_TO);
				SER_loadTxTerminate();
				phaseCtr++;
			}
			else
			{
				if(rcvdResp == AT_R_STA_CUR_IP)
				{
					flags |= BIT0;
					strcpy(ESP.Self_IP,esp_argv_buff);
				}
				else if (rcvdResp == AT_R_STA_CUR_GTW)
				{
					flags |= BIT1;
					strcpy(ESP.Gtw_IP,esp_argv_buff);
				}
				else if(rcvdResp == AT_R_STA_CUR_NET_MSK)
				{
					flags |= BIT2;
					strcpy(ESP.Net_Mask,esp_argv_buff);
				}
				else if(rcvdResp == AT_R_OK)
				{
					//sprintf(hex,"\n%s",ESP.Self_IP);	DEBUG_sendHex2Ch(3);// SER_loadTxString(serPort, hex ,0, SER_LOAD_TX_TO);
					//sprintf(hex,"--%s",ESP.Gtw_IP);		DEBUG_sendHex2Ch(3);//SER_loadTxString(serPort, hex ,0, SER_LOAD_TX_TO);
					//sprintf(hex,"--%s",ESP.Net_Mask);	DEBUG_sendHex2Ch(3);//SER_loadTxString(serPort, hex ,0, SER_LOAD_TX_TO);
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
				SER_loadTxString(serPort, CMD_OUT[AT_T_CONFIG_TCP_SERVER] ,0, SER_LOAD_TX_TO);
				SER_loadTxString(serPort, "0" ,0, SER_LOAD_TX_TO);
				SER_loadTxTerminate();
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
				SER_loadTxString(serPort, CMD_OUT[AT_T_SET_MULTI_CONNECT] ,0, SER_LOAD_TX_TO);
				SER_loadTxString(serPort, "0" ,0, SER_LOAD_TX_TO);  //###
				SER_loadTxTerminate();
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
				SER_loadTxString(serPort, CMD_OUT[AT_T_SET_ADD_PORT_RX_PKT] ,0, SER_LOAD_TX_TO);
				SER_loadTxString(serPort, "1" ,0, SER_LOAD_TX_TO);
				SER_loadTxTerminate();
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
				SER_loadTxString(serPort, CMD_OUT[AT_T_LIST_AVLB_AP] ,0, SER_LOAD_TX_TO);
				SER_loadTxString(serPort, "\"R&D-System\"" ,0, SER_LOAD_TX_TO);//AT_T_SET_HOSTNAME
				SER_loadTxTerminate();
			break;
		}*/
	}
	if(seqCtr > 9)
	{
		ESP.State = ESP_IDLE;
		ESP_rqstConnection(NULL, CONN_UDP_PROXY, "192.168.1.40", 6000, 0, 0, CRQ_FLAG_PROXY);
		phaseCtr = 0;
	}

	return phaseCtr;
}

static uint8_t espStateFnUDPProxy( uint8_t rcvdResp)
{
	static uint16_t byteCounter = 0;
	char 	txData = 0;
	uint32_t	status = 0;
	//TickType_t xLastWakeTime;


	if(xTaskNotifyWait(0x00,0x00,&status, portMAX_DELAY) == pdFAIL)
	{
		return RC_FAILURE;
	}
	/*else if(status != 0x89AB)
	{
		return RC_FAILURE;
	}*/

	while(xQueueReceive(proxyTxQueueHandle, &txData, 0))
	{
		SER_loadTxString(serPort, &txData, 1, SER_LOAD_TX_TO);
		//xLastWakeTime = xTaskGetTickCount(); //Get current time
		byteCounter++;

		if(byteCounter == 2048)
		{
			byteCounter 	= 0;
			vTaskDelay(pdMS_TO_TICKS(20));	//or we can drop these packets if needed
			//vTaskDelayUntil(&xLastWakeTime,pdMS_TO_TICKS(200));
		}
	}

	return RC_SUCCESS;
}

result_code_t ESP_txDataOverUDPProxy(uint8_t linkId, uint8_t *txData, uint16_t size)
{
	//DEBUG_FN_ENTRY();
	/*if(espConnections[linkId].connStat == CONN_STAT_PROXY)
	{
		sprintf(hex, "#A "); DEBUG_sendHex3;
		return RC_FAILURE;
	}*/


	uint8_t i=0;
	for(i=0 ; i<size ; i++)
	{
		if (xQueueSend(proxyTxQueueHandle, &txData[i], 423) == pdFAIL)
		{
			return RC_FAILURE;
			sprintf(hex, "#B "); DEBUG_sendHex3;
		}
	}
    xTaskNotify(espTaskHandle,0x89AB,eSetValueWithOverwrite);

	return RC_SUCCESS;
}

result_code_t ESP_closeUDPProxy(uint8_t linkId)
{
	//DEBUG_FN_ENTRY();

	//if(espConnections[linkId].connStat == CONN_STAT_PROXY)	return RC_FAILURE;  //ToDo: this needs to be used but failing

	TaskHandle_t task = xTaskGetCurrentTaskHandle();
	if(task == NULL)
	{
		return RC_FAILURE;
	}


	while(uxQueueMessagesWaiting(proxyTxQueueHandle))
	{
		vTaskDelay(10); //This will also prevent the application task from loading more data in Proxy Mode
	}

	//espCloseProxyChannels();
	espConnections[linkId].connStat = CONN_STAT_TRANSIENT;
	ESP.State = ESP_CLOSE_UDP_PROXY;	//End transparent mode		//this needs to be done through request mechanism actually

	return RC_SUCCESS;
}

/*static void espCloseProxyChannels(void)
{
	//This guard time is needed.
	// Guard time before +++ is needed - ChatGPT. No proof found but this time is needed as found experimentally.
	vTaskDelay(pdMS_TO_TICKS(1000));
	SER_loadTxString(serPort,"+++",0, 1);
	// Guard time after +++ is needed. This is what Espressif says.
	vTaskDelay(pdMS_TO_TICKS(1000));

	SER_loadTxString(serPort, CMD_OUT[AT_T_CIPCLOSE] ,0, SER_LOAD_TX_TO);
	SER_loadTxString(serPort, "\r\n" ,2, SER_LOAD_TX_TO);

	vTaskDelay(pdMS_TO_TICKS(1000));
	SER_loadTxString(serPort, CMD_OUT[AT_T] ,0, SER_LOAD_TX_TO);
	SER_loadTxString(serPort, "\r\n" ,2, SER_LOAD_TX_TO);

}*/

static uint8_t espStateFnCloseUDPProxy( uint8_t rcvdResp, esp_con_rqst_t *connRqst)
{
	//DEBUG_FN_ENTRY();
	static uint8_t seqCtr=0x00;
	static uint8_t phaseCtr=0x00;

	switch(seqCtr)
	{
		case 0:
		{
			if(!phaseCtr)
			{
				espConnections[0].connStat = CONN_STAT_TRANSIENT;

				// This guard time is needed.
				// ChatGPT - Guard time before +++ is needed. No proof found but this time is actually needed (confirmed experimentally).
				vTaskDelay(pdMS_TO_TICKS(1000));
				SER_loadTxString(serPort,"+++",0, 1);
				vTaskDelay(pdMS_TO_TICKS(1000)); // Guard time after +++ is needed. This is what Espressif says.
				phaseCtr = 0;	//So that the espGetAtResponse function doesn't look for response
				seqCtr++;
				break;
			}
			break;
		}
		case 1:
		{
			if(!phaseCtr)
			{
				//Send Command
				SER_loadTxString(serPort, CMD_OUT[AT_T_CIPCLOSE] ,0, SER_LOAD_TX_TO);
				SER_loadTxTerminate();
				phaseCtr++;

				break;
			}
			else
			{
				if(rcvdResp == AT_R_CLOSED)
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

		default:
		{
			break;
		}
	}
	if(seqCtr > 1)
	{
		ESP.State = ESP_CONNECTED_IDLE;
		espConnections[0].connStat = CONN_STAT_CLOSED;
		seqCtr	=	0;
		phaseCtr=	0;
	}

	return phaseCtr;
}

static uint8_t espStateFnOpenUDPProxy( uint8_t rcvdResp, esp_con_rqst_t *connRqst)
{
	//DEBUG_FN_ENTRY();
	static uint8_t seqCtr=0x00;
	static uint8_t phaseCtr=0x00;

	switch(seqCtr)
	{
		case 0:
		{
			if(!phaseCtr)
			{
				/*connRqst->linkId = espGetFreeLinkId();
				if(connRqst->linkId == 0xFF)
				{
					seqCtr	=	0;
					phaseCtr=	0;
					ESP.State = ESP_CONNECTION_ESTD;	//Send to some error state
					return RC_FAILURE;
				}*/
				espConnections[connRqst->linkId].connStat = CONN_STAT_TRANSIENT;


				SER_loadTxString(serPort,		CMD_OUT[AT_T_SETUP_CONNECTION]	,0, SER_LOAD_TX_TO);
				//sprintf(hex,"%d,", connRqst->linkId);
				//SER_loadTxString(serPort,		hex								,0, SER_LOAD_TX_TO);
				//Connection type : UDP
				SER_loadTxString(serPort,		"\"UDP\",\""					,0, SER_LOAD_TX_TO);
				//Send Host IP Address
				SER_loadTxString(serPort,		connRqst->ipAddress				,0, SER_LOAD_TX_TO);
				SER_loadTxString(serPort,		"\","							,0, SER_LOAD_TX_TO);
				// Send Remote and Local port Numbers
				sprintf(hex,"%d", connRqst->port);
				SER_loadTxString(serPort,		hex								,0, SER_LOAD_TX_TO);
				SER_loadTxString(serPort,		","								,0, SER_LOAD_TX_TO);
				SER_loadTxString(serPort,		"50000"							,0, SER_LOAD_TX_TO);
				SER_loadTxString(serPort,		",0"							,0, SER_LOAD_TX_TO);	//0 for proxy mode
				SER_loadTxTerminate();
				phaseCtr++;
				break;
			}
			else
			{
				if((rcvdResp == AT_R_ALREADY_CONN)	||
						(rcvdResp == AT_R_CONNECT)	||
						(rcvdResp == AT_R_LINK_CONNECT_0)||
						(rcvdResp == AT_R_LINK_CONNECT_1)||
						(rcvdResp == AT_R_LINK_CONNECT_2)||
						(rcvdResp == AT_R_LINK_CONNECT_3))
				{
					phaseCtr++;
				}
				else if((rcvdResp == AT_R_OK) || (rcvdResp == AT_R_ERR) )
				{
					phaseCtr = 0;
					seqCtr++;
				}
			}
			break;
		}
		case 1:
		{
			if(!phaseCtr)
			{
				//Send Command
				SER_loadTxString(serPort, CMD_OUT[AT_T_SET_CIPMODE]		,0, SER_LOAD_TX_TO);
				SER_loadTxString(serPort, "=1" 							,0, SER_LOAD_TX_TO);
				SER_loadTxTerminate();
				phaseCtr++;

				break;
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
		case 2:
		{
			if(!phaseCtr)
			{
				//Send Command
				SER_loadTxString(serPort,		CMD_OUT[AT_T_SEND_DATA]			,0, SER_LOAD_TX_TO);
				SER_loadTxTerminate();
				phaseCtr++;

				break;
			}
			else
			{
				if(rcvdResp == AT_R_OK)
				{
					phaseCtr++;
				}
				else if(rcvdResp == AT_R_RDY_2_ACCP_TXD)
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
	}
	if(seqCtr > 2)
	{
		ESP.State = ESP_UDP_PROXY;
		seqCtr	=	0;
		phaseCtr=	0;
		//espRegisterSocket(&connRqst);
		espConnections[0].connStat = CONN_STAT_PROXY;

		/*sprintf(hex,"maneesh");
		uint8_t len = 0;
		len = strlen(hex);

		ESP_txDataOverUDPProxy(0,hex, len);*/
	}

	return phaseCtr;
}

static uint8_t espStateFnStartUDPClient( uint8_t rcvdResp, esp_con_rqst_t *connRqst)
{
	//DEBUG_FN_ENTRY();
	static uint8_t seqCtr=0x00;
	static uint8_t phaseCtr=0x00;

	switch(seqCtr)
	{
		case 0:
		{
			if(!phaseCtr)
			{
				connRqst->linkId = espGetFreeLinkId();
				if(connRqst->linkId == 0xFF)
				{
					seqCtr	=	0;
					phaseCtr=	0;
					ESP.State = ESP_CONNECTION_ESTD;	//Send to some error state
					return RC_FAILURE;
				}


				SER_loadTxString(serPort,		CMD_OUT[AT_T_SETUP_CONNECTION]	,0, SER_LOAD_TX_TO);
				sprintf(hex,"%d,", connRqst->linkId);
				SER_loadTxString(serPort,		hex								,0, SER_LOAD_TX_TO);
				//Connection type : UDP
				SER_loadTxString(serPort,		"\"UDP\",\""					,0, SER_LOAD_TX_TO);
				//Send Host IP Address
				SER_loadTxString(serPort,		connRqst->ipAddress				,0, SER_LOAD_TX_TO);
				SER_loadTxString(serPort,		"\","							,0, SER_LOAD_TX_TO);
				// Send Remote and Local port Numbers
				sprintf(hex,"%d", connRqst->port);
				SER_loadTxString(serPort,		hex								,0, SER_LOAD_TX_TO);
				SER_loadTxString(serPort,		","								,0, SER_LOAD_TX_TO);
				SER_loadTxString(serPort,		"50000"							,0, SER_LOAD_TX_TO);
				SER_loadTxString(serPort,		",2"						,0, SER_LOAD_TX_TO);
				SER_loadTxTerminate();
				phaseCtr++;
				break;
			}
			else
			{
				if((rcvdResp == AT_R_ALREADY_CONN)	||
						(rcvdResp == AT_R_CONNECT)	||
						(rcvdResp == AT_R_LINK_CONNECT_0)||
						(rcvdResp == AT_R_LINK_CONNECT_1)||
						(rcvdResp == AT_R_LINK_CONNECT_2)||
						(rcvdResp == AT_R_LINK_CONNECT_3))
				{
					phaseCtr++;
				}
				else if((rcvdResp == AT_R_OK) || (rcvdResp == AT_R_ERR) )
				{
					phaseCtr = 0;
					seqCtr++;
				}
			}
			break;
		}
		/*case 2:		// Dummy case
		{
			if(!phaseCtr)
			{
				//SER_loadTxString(serPort, CMD_OUT[AT_T_CHK_CUR_MODE] ,0, SER_LOAD_TX_TO);
				SER_loadTxString(serPort, CMD_OUT[AT_T_SET_CUR_MODE] ,0, SER_LOAD_TX_TO);
				SER_loadTxString(serPort, "1" ,1, SER_LOAD_TX_TO);
				SER_loadTxTerminate();
				phaseCtr++;
			}
			else
			{
				if(rcvdResp == AT_R_CUR_MODE)
				{
					phaseCtr++;
				}
				else if(rcvdResp == AT_R_ACK)
				{
					phaseCtr = 0;
					seqCtr++;
				}
			}
			break;
		}*/

		default:
		{
			break;
		}
	}
	if(seqCtr > 0)
	{
		ESP.State = ESP_CONNECTION_ESTD;
		seqCtr	=	0;
		phaseCtr=	0;

	}

	return phaseCtr;
}

static uint8_t espStateFnTxInProgress( uint8_t rcvdResp, esp_dat_rqst_t *datRqst )
{
	//DEBUG_FN_ENTRY();
	static uint8_t seqCtr=0;
	static uint8_t phaseCtr=0;

	//sprintf(hex, "#A%d,%d",seqCtr,phaseCtr ); SEGGER_SYSVIEW_PrintfHost(hex);

	switch(seqCtr)
	{
		case 0:
		{
			if(!phaseCtr)
			{
				//Send Command
				SER_loadTxString(serPort,		CMD_OUT[AT_T_SEND_DATA]				,0, SER_LOAD_TX_TO);

				//Send LinkId
				sprintf(hex,"=%d,", datRqst->linkId);
				SER_loadTxString(serPort,		hex									,0, SER_LOAD_TX_TO);

				//Send length of data
				sprintf(hex,"%d,\"", datRqst->size);
				SER_loadTxString(serPort,		hex									,0, SER_LOAD_TX_TO);

				//Send Host IP Address
				SER_loadTxString(serPort,		datRqst->ipAddress					,0, SER_LOAD_TX_TO);	//
				SER_loadTxString(serPort,		"\","								,0, SER_LOAD_TX_TO);

				// Send Remote and Local port Numbers
				sprintf(hex,"%d", espConnections[0].port);
				SER_loadTxString(serPort,		hex									,0, SER_LOAD_TX_TO);

				SER_loadTxTerminate();
				phaseCtr++;

				break;
			}
			else
			{
				if(rcvdResp == AT_R_OK)
				{
					phaseCtr++;
				}
				else if(rcvdResp == AT_R_RDY_2_ACCP_TXD)
				{
					phaseCtr = 0;
					seqCtr++;
				}
			}
			break;
		}
		case 1:		// Dummy case
		{
			if(!phaseCtr)
			{
				//char val = '!';
				//sprintf(hex, "Hello%c\r",val);		SER_loadTxString(serPort, hex ,7, SER_LOAD_TX_TO);

				SER_loadTxString(serPort, datRqst->buffer ,datRqst->size, SER_LOAD_TX_TO);
				phaseCtr++;
			}
			else
			{
				if(rcvdResp == AT_R_RCVD_BYTES)
				{
					phaseCtr++;
				}
				else if(rcvdResp == AT_R_UNKWN_CMD)
				{
					phaseCtr++;
				}
				else if(rcvdResp == AT_R_SEND_OK)
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
	}

	if(seqCtr > 1)
	{
		ESP.State = ESP_CONNECTED_IDLE;
		seqCtr	=	0;
		phaseCtr=	0;
	}

	return phaseCtr;
}

#endif

static result_code_t espRegisterSocket(esp_con_rqst_t *connRqst)
{
	if(connRqst->linkId >= MAX_SUPPORTED_CONNECTIONS)
		return RC_FAILURE;

	uint8_t linkId = connRqst->linkId;

	if(espConnections[linkId].ConnType == __CONN_TYPE_MAX)
	{
		// Copying basic connection details
		espConnections[linkId].ConnType     = connRqst->connType;
		espConnections[linkId].port         = connRqst->port;
		espConnections[linkId].linkId       = linkId;

		// Copying IP address with null-termination
		strncpy(espConnections[linkId].targetIP, connRqst->ipAddress, 15);
		espConnections[linkId].targetIP[15] = '\0';

		// Setting task and optional flags
		espConnections[linkId].task         = connRqst->task;
		//espConnections[linkId].flags        = connRqst->flags;

		// Initialize timeouts and queues
		//espConnections[linkId].connTO       	= connRqst->timeoutMs;
		espConnections[linkId].connStat     	= CONN_STAT_IDLE; // Default initial state
		espConnections[linkId].dataFormat   	= 0;              // Default to binary format

		// Initialize queue lengths and create queues
		if(connRqst->flags & CRQ_FLAG_TX)
		{
			espConnections[linkId].txQueueLength = DEFAULT_TX_QUEUE_LENGTH;
			espConnections[linkId].txQueueHandle = xQueueCreate(
					espConnections[linkId].txQueueLength, sizeof(uint8_t));
			// Check if queue creation was successful
			if (espConnections[linkId].txQueueHandle == NULL)
			{
				// Cleanup if queue creation fails
				vQueueDelete(espConnections[linkId].txQueueHandle);
				return RC_FAILURE;
			}
		}

		if(connRqst->flags & CRQ_FLAG_RX)
		{
			espConnections[linkId].rxQueueLength = DEFAULT_RX_QUEUE_LENGTH;
			espConnections[linkId].rxQueueHandle = xQueueCreate(
					espConnections[linkId].rxQueueLength, sizeof(uint8_t));
			// Check if queue creation was successful
			if (espConnections[linkId].rxQueueHandle == NULL)
			{
				// Cleanup if queue creation fails
				vQueueDelete(espConnections[linkId].rxQueueHandle);
				return RC_FAILURE;
			}
		}

		if(connRqst->flags & CRQ_FLAG_PROXY)
		{
			espConnections[linkId].connStat = CONN_STAT_PROXY;
		}

		return RC_SUCCESS;
	}

	return RC_FAILURE;
}

static void rxResponseTimeoutEvent(void)
{
	;//Implement later
}


//Data and Control requests processing functions
#if  1
result_code_t espConRqstHandler( esp_con_rqst_t *connRqst )
{
	if(espChkIPAddValidity(connRqst->ipAddress) == RC_FAILURE)
	{
		return RC_FAILURE;
	}


	switch(connRqst->connType)
	{
		case CONN_TCP_SERVER:
		{
			ESP.State = ESP_START_TCP_SERVER;
			break;
		}
		case CONN_TCP_CLIENT:
		{
			ESP.State = ESP_START_TCP_CLIENT;
			break;
		}
		case CONN_UDP_SERVER:
		case CONN_UDP_CLIENT:
		{
			ESP.State = ESP_START_UDP_CLIENT;
			break;
		}
		case CONN_UDP_PROXY:
		{
			ESP.State = ESP_OPEN_UDP_PROXY;
			break;
		}
		default:
		{
			break;
		}
	}
	return RC_SUCCESS;
}

result_code_t espDatRqstHandler( esp_dat_rqst_t *datRqst )
{
	//DEBUG_FN_ENTRY();
	if(espChkIPAddValidity(datRqst->ipAddress) == RC_FAILURE)
	{
		return RC_FAILURE;
	}

	if(datRqst->flags & DRQ_FLAG_TX)	//Data is to be transmitted thru socket
	{

		ESP.State = ESP_TXIP;
	}
	else	//Data from socket is to be delivered to requesting task
	{

	}

	return RC_SUCCESS;
}
#endif


//Functions to get Data from request queue
#if 1
result_code_t espGetDatRqstFromQueue(esp_dat_rqst_t *datRqst)
{
    // Validate the input pointer
    if (datRqst == NULL)
    {
        return RC_FAILURE; // Invalid pointer
    }

    // Try to dequeue a request from the queue
    if (xQueueReceive(datRqstQueueHandle, datRqst, RQST_QUEUE_TO) == pdTRUE)
    {
        return RC_SUCCESS; // Successfully dequeued
    }

    return RC_FAILURE; // No data available in the queue
}

result_code_t espGetConRqstFromQueue(esp_con_rqst_t *connRqst)
{
    // Check if the queue has any messages
    /*if (uxQueueMessagesWaiting(RqstQueueHandle) == 0)
    {
        return RC_FAILURE; // Queue is empty
    }*/

#if (ESP_EN_RQST_PRIORITY == 1)
    esp_con_rqst_t highestPriorityRqst;  // To store the highest priority request
    esp_con_rqst_t tempRqst;             // Temporary buffer for dequeued items
    uint8_t isFirstItem = 1;              // Flag to identify the first item checked
    UBaseType_t itemsInQueue = uxQueueMessagesWaiting(conRqstQueueHandle);

    // Step 1: Iterate through the queue to find the highest priority request
    for (UBaseType_t i = 0; i < itemsInQueue; i++)
    {
        // Receive the front item
        if (xQueueReceive(conRqstQueueHandle, &tempRqst, 0) == pdPASS)
        {
            // Update the highest priority request
            if (isFirstItem || tempRqst.priority > highestPriorityRqst.priority)
            {
                highestPriorityRqst = tempRqst;
                isFirstItem = 0;
            }

            // Re-enqueue the item to maintain queue order
            xQueueSendToBack(conRqstQueueHandle, &tempRqst, 0);
        }
    }

    // Copy the highest priority request to the provided pointer
    *connRqst = highestPriorityRqst;
    return RC_SUCCESS;

#else// Normal FIFO dequeue logic

    // Receive the front item directly into the provided pointer
    if (xQueueReceive(conRqstQueueHandle, connRqst, RQST_QUEUE_TO) == pdPASS)
    {
        return RC_SUCCESS; // Successfully dequeued
    }

    return RC_FAILURE; // Timeout occurred
#endif
}
#endif


//Functions to register Control and Data Requests
#if 1
uint8_t ESP_rqstDataTransfer(TaskHandle_t task, uint8_t linkId, uint8_t priority, uint8_t flags,
		uint16_t port, char *ipAddress,
		uint8_t *buffer, uint16_t size)
{
    esp_dat_rqst_t rqst;

    // Validate inputs
    if (linkId >= MAX_SUPPORTED_CONNECTIONS || size == 0 || buffer == NULL)
    {
        return RC_FAILURE; // Invalid input
    }

    // Populate the request structure
    rqst.task 		= (task != NULL) ? task : xTaskGetCurrentTaskHandle();
    rqst.linkId 	= linkId;
    rqst.buffer 	= buffer;
    rqst.size 		= size;
    rqst.priority 	= priority;
    rqst.flags 		= flags;
    rqst.port 		= port;

    strncpy(rqst.ipAddress, ipAddress, 15);
    rqst.ipAddress[15] = '\0'; // Ensure null-termination.

    rqst.timeStamp = SYS_getCurrentTime();

    // Enqueue the request
	if (xQueueSend(datRqstQueueHandle, &rqst, 0) != pdTRUE)
	{
		return RC_FAILURE; // Queueing failed
	}

	//Tell the ESP task about pending request
	xSemaphoreGive(pendingRqstSemaphore);			//xTaskNotify(espTaskHandle, 1, eIncrement);

	//sprintf(hex, "#B " ); SEGGER_SYSVIEW_PrintfHost(hex);
	return RC_SUCCESS; //
}

uint8_t ESP_rqstConnection(TaskHandle_t task, esp_conn_types connType, char *ipAddress, uint16_t port, uint16_t timeoutMs, uint8_t priority, uint8_t flags)
{
	esp_con_rqst_t rqst;

    // Step 1: Handle NULL task
    if (task == NULL) {
        task = xTaskGetCurrentTaskHandle(); // FreeRTOS API to get current task handle
    }
    rqst.task = task;

    // Step 2: Save connection type
    rqst.connType = connType;

    // Step 3: Save IP address
    strncpy(rqst.ipAddress, ipAddress, sizeof(rqst.ipAddress) - 1);
	rqst.ipAddress[sizeof(rqst.ipAddress) - 1] = '\0'; // Ensure null termination

    // Step 4: Save port
    rqst.port = port;

    // Step 5: Save timeout
    rqst.timeoutMs = timeoutMs;

    // Step 6: Save priority
    rqst.priority = priority;

    // Step 7: Save flags
    rqst.flags = flags;

    // Step 8: Push to queue
    if (xQueueSend(conRqstQueueHandle, &rqst, pdMS_TO_TICKS(100)) != pdPASS)
    {
        return FAIL; // Queue send failed
    }

    //Tell the ESP task about pending request
	xSemaphoreGive(pendingRqstSemaphore);			//xTaskNotify(espTaskHandle, 1, eIncrement);
    return PASS; // Success
}


#endif

//helper functions
#if 1
static uint8_t espGetFreeLinkId( void )
{
	uint8_t i=0;
	for(i=0 ; i<MAX_SUPPORTED_CONNECTIONS ; i++)
	{
		if(espConnections[i].ConnType == __CONN_TYPE_MAX)
		{
			return i;
		}
	}

	return 0xFF;
}

result_code_t espChkIPAddValidity(char *ipAddress)
{
    if (ipAddress == NULL)
    {
        return RC_FAILURE;
    }

    int num = 0;       // Current number being processed
    int dots = 0;      // Dot count
    int digits = 0;    // Digits in the current segment

    while (*ipAddress)
    {
        // Check if the current character is a digit
        if (*ipAddress >= '0' && *ipAddress <= '9')
        {
            num = num * 10 + (*ipAddress - '0'); // Accumulate the number
            digits++;

            // Check for out-of-range number or too many digits
            if (num > 255 || digits > 3)
            {
                return RC_FAILURE;
            }
        }
        else if (*ipAddress == '.')
        {
            // Validate the segment before the dot
            if (digits == 0 || digits > 3 || num > 255)
            {
                return RC_FAILURE;
            }

            dots++;
            num = 0;      // Reset for the next segment
            digits = 0;   // Reset digit count
        }
        else
        {
            // Invalid character detected
            return RC_FAILURE;
        }

        ipAddress++;
    }

    // Validate the last segment after the final dot
    if (dots != 3 || digits == 0 || digits > 3 || num > 255)
    {
        return RC_FAILURE;
    }

    return RC_SUCCESS;
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

		//Receive all bytes until the Serial Queue is empty
		while(SER_getRxData(serPort, &rxData, SER_GET_RX_TO))
		{
			esp_rx_buff[buffPtr] = rxData;
			buffPtr++;
			if (rxData == '\r' || rxData == '\n' || rxData == '>')
			{
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
				/*sprintf(hex,"Cm:%d",detCmd);DEBUG_sendHex2Ch(3);
				if(detCmd == AT_R_UNKWN_CMD)
				{
					sprintf(hex,"\"%s\"",esp_rx_buff);DEBUG_sendHex2Ch(3);
				}*/
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

bool ESP_getModuleInitStat( void )
{
	return moduleInit;
}
#endif
