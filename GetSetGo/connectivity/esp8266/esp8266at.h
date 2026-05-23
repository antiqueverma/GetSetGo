/*
 * ESP_AT.h
 *
 *  Created on: Dec 24, 2024
 *      Author: antiq
 */

#ifndef ESP8266AT_H_
#define ESP8266AT_H_

//	#define CMD_NUM		8
	#define ESP_CMD_MAXLEN		40		//Maximum length of input commands
	#define PARAM_LEN_INT 		4     //Maximum length of a received parameter (number of chars)
	#define PARAM_LEN_STR 		5     //Maximum length of a received parameter (number of chars)

	#define CMD_PARAM_PRFX		':'		//Parameters always begin after this indicator
	#define CMD_TRM1			'\r'
	#define CMD_TRM2			'\n'

	#define WAIT 				0x01
	#define FOUND 				0x02
	#define NOT_FOUND 			0x00


	/****** AT Command Indexes ******/
	typedef enum{
		AT_T = 0,
		AT_T_VERSION,
		AT_T_ECHO_OFF,
		AT_T_SET_BAUD_CUR,
		AT_T_CHK_CUR_MODE,
		AT_T_SET_CUR_MODE,
		AT_T_LIST_AVLB_AP,
		AT_T_CONNECT_2_AP_CUR,
		AT_T_CONNECT_2_AP_DEF,
		AT_T_CHK_CUR_AP,
		AT_T_DISCON_AP,
		AT_T_SET_HOSTNAME,
		AT_T_CHK_CUR_IP,
		AT_T_START_CON,
		AT_T_START_MDNS,
		AT_T_SET_HOST_NAME,
		AT_T_CONFIG_TCP_SERVER,
		AT_T_TCP_CL_DCON_TO,
		AT_T_SETUP_CONNECTION,
		AT_T_RESET,
		AT_T_SEND_DATA,
		AT_T_SET_MULTI_CONNECT,
		AT_T_SET_ADD_PORT_RX_PKT,
		AT_T_SET_CIPMODE,
		AT_T_CIPCLOSE,


		//Always keep below entry as last
		__AT_T_LASTCMD
	} espat_tx_cmd_t;

	typedef enum
	{
		AT_R_OK = 0,
		AT_R_ERR,
		AT_R_BUSY,
		AT_R_CUR_MODE,
		AT_R_NO_AP,
		AT_R_WIFI_CONN,
		AT_R_WIFI_DISCON,
		AT_R_STA_CUR_IP,
		AT_R_STA_CUR_GTW,
		AT_R_STA_CUR_NET_MSK,
		AT_R_CUR_AP,
		AT_R_GOT_IP,
		AT_R_RX_DATA,
		AT_R_NO_CHANGE,
		AT_R_LINK_ALREADY_BUILT,
		AT_R_ALREADY_CONN,
		AT_R_CONNECT,
		AT_R_LINK_CONNECT_0,
		AT_R_LINK_CONNECT_1,
		AT_R_LINK_CONNECT_2,
		AT_R_LINK_CONNECT_3,
		AT_R_LINK_CLOSED,
		AT_R_CLOSED,
		AT_R_RDY_2_ACCP_TXD,
		AT_R_RCVD_BYTES,
		AT_R_SEND_OK,
		AT_R_BUSY_S,
		AT_R_ATVERSION,


		__AT_R_LASTCMD,         // Marks the last valid command
		AT_R_Q_EMPTY = 0xFE,
		AT_R_UNKWN_CMD = 0xFF    // Unknown command
	} espat_rx_cmd_t;


extern char *ESP_CMD_OUT[__AT_T_LASTCMD];
extern char *ESP_CMD_IN[__AT_R_LASTCMD];


#endif /* ESP8266AT_H_ */
