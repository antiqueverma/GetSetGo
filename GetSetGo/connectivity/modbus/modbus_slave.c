#include "modbus.h"

gsg_result_t mbPhyPreRx(modbus_port_t *port, uint8_t *data, uint16_t size);
gsg_result_t mbPhySendData(modbus_port_t *port, uint8_t *data, uint16_t size);
void mbTxGenFrame(modbus_port_t *port, mb_query_type_t queryType, uint8_t slaveId, uint16_t address, uint16_t regCount);
modbus_error_t mbRxFrameParse(modbus_port_t *port, uint8_t slaveId, modbus_func_code_t funcCode, uint16_t address, uint16_t regCount);


void modbusSlaveTaskHandler(void * argument)
{
    // typecast the port parameter to modbus_port_t pointer
    modbus_port_t *modbusPort = (modbus_port_t *)argument;
    char tempBuffer[100]; // Temporary buffer for debug messages
    modbusPort->state = MB_PORT_STATE_SLAVE_IDLE; // Set initial state to disabled
    mbPhyRxCbContext_t *ctx = NULL;
    mb_config_request_t configRqst;
//    mb_master_query_t   queryInProcess;




    // Modbus task implementation
    while (1)
    {
        static uint8_t prvState = 0xFF;
        if(modbusPort->state != prvState)
        {
            prvState = modbusPort->state; // Update previous state
            // sprintf(tempBuffer,"State: %d", modbusPort->state);
            // DEBUG_LOGI(DEBUG_TAG_MODBUS,"MBS",tempBuffer);
        }

        // Modbus port state machine
        switch (modbusPort->state)
        {
            // Master Mode States
            case MB_PORT_STATE_SLAVE_IDLE:
            {
            	// Check for any configuration requests and process them all at once
				while (xQueueReceive(modbusPort->configRqstQueHandle, &configRqst, 0) == pdTRUE)
				{
                    sprintf(tempBuffer, "New Rqst:%d", configRqst.rqstType);
                    DEBUG_LOGD(DEBUG_TAG_MODBUS,"MBS",tempBuffer);

					// Process the configuration request
					// mb_slaveProcessConfigRequest(modbusPort, &configRqst);
				}
                modbusPort->state = MB_PORT_STATE_SLAVE_RX_WAITING;
                break;
            }
            case MB_PORT_STATE_SLAVE_RX_WAITING:
            {
                uint8_t byte = 0;
                if (xQueueReceive(ctx->rxQueueHandle, &byte, pdMS_TO_TICKS(MODBUS_SLAVE_RX_QUERY_TIMEOUT_MS)) == pdTRUE)
                {
                	modbusPort->rx_buffer[modbusPort->rx_buffer_length++] = byte;
					while (xQueueReceive(ctx->rxQueueHandle, &byte, pdMS_TO_TICKS(MODBUS_MASTER_INTER_BYTE_TIMEOUT_MS)) == pdTRUE)
					{
						if (modbusPort->rx_buffer_length < sizeof(modbusPort->rx_buffer))
						    modbusPort->rx_buffer[modbusPort->rx_buffer_length++] = byte;
						else
						    DEBUG_LOGE(DEBUG_TAG_MODBUS, "MBS", "RX buffer overflow");
					}
                }

                // Timeout happened, check what was received so far
                if(modbusPort->rx_buffer_length == 0)
                {
                    DEBUG_LOGW(DEBUG_TAG_MODBUS, "MBS", "No Query from Master");
                    modbusPort->state = MB_PORT_STATE_SLAVE_RESET;
                }
                else
                {	
//                    mbPhyPreRx(modbusPort, modbusPort->rx_buffer, byte);
                    // mbRxFrameParse(modbusPort, 
                    //    queryInProcess.slaveId,
                    //     modbusPort->tx_buffer[1], 
                    //     queryInProcess.address, 
                    //     queryInProcess.regCount);
                    modbusPort->state = MB_PORT_STATE_SLAVE_RX_PROCESSING;
                } 
                
                break;
            }
            case MB_PORT_STATE_SLAVE_TX_PROCESSING:
            {
            	modbus_phy_t phy = modbusPort->slaveData->phy;
				ctx = modbusPort->rxCtx[phy];
				if (ctx == NULL || ctx->rxQueueHandle == NULL)
				{
					DEBUG_LOGE(DEBUG_TAG_MODBUS, "MBS", "No valid Rx context");
					modbusPort->state = MB_PORT_STATE_MASTER_RESET;
					break;
				}
				xQueueReset (ctx->rxQueueHandle);
				modbusPort->rx_buffer_length = 0;

//                mbTxGenFrame(modbusPort,
//                                queryInProcess.type,
//                                queryInProcess.slaveId,
//                                queryInProcess.address,
//                                queryInProcess.regCount);

                modbusPort->state = MB_PORT_STATE_MASTER_TRANSMITTING;
                break;
            }
            case MB_PORT_STATE_SLAVE_TRANSMITTING:
            {
                mbPhySendData(modbusPort,
                                modbusPort->tx_buffer,
                                modbusPort->tx_buffer_length);
                                
                modbusPort->state = MB_PORT_STATE_MASTER_RX_WAITING;
                break;
            }
            case MB_PORT_STATE_SLAVE_RESET:
            {
                // Reset the Modbus port state      
                modbusPort->state = MB_PORT_STATE_SLAVE_IDLE; // Reset to idle state
                break;
            }
            case MB_PORT_STATE_DISABLED:
            {
                // Port is disabled, do nothing
                break;
            }
            default:
                break;
        }
    }
    vTaskDelete(NULL); // Exit the task when done
}
