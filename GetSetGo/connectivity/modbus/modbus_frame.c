#include "modbus.h"

modbus_error_t mb_regWrite(modbus_slave_info_t *slave, mb_query_type_t queryType, uint16_t regAddress, uint8_t regCount, uint8_t *value);
modbus_error_t mb_regRead(modbus_slave_info_t *slave, mb_query_type_t queryType, uint16_t regAddress, uint8_t regCount, uint8_t *value);
modbus_slave_info_t *mbGetSlaveInfo(modbus_port_t *port, uint8_t slaveId);
uint16_t mbframeCalculateCRC16(uint8_t *data, uint16_t size);


void mbTxGenFrame(modbus_port_t *port, mb_query_type_t queryType, uint8_t slaveId, uint16_t address, uint16_t regCount)
{
    if (port == NULL) {
        return; // Invalid parameters
    }

    // Prepare the Modbus frame
    uint8_t *frame = port->tx_buffer;
    size_t frame_length = 0;
    modbus_slave_info_t *slave = mbGetSlaveInfo(port, slaveId);

    // Add the Modbus PDU (Protocol Data Unit) based on the query type
    frame[0] = slaveId;

//	char hex[100];	sprintf(hex, "Id=%d, Type=%d, Address=%d, RegCount=%d", slaveId, queryType, address, regCount);	DEBUG_LOGI(DEBUG_TAG_COMM,"MB", hex);

    switch (queryType)
    {
        case MB_QUERY_READ_COILS:
        {
            DEBUG_LOGE(DEBUG_TAG_COMM,"MB", "Read coils NYI");
            vTaskSuspend(NULL);
            // Build the frame for reading coils
            frame[1] = MB_FUNC_READ_COILS;
            frame[2] = (address >> 8) & 0xFF;
            frame[3] = address & 0xFF;
            frame[4] = (regCount >> 8) & 0xFF;
            frame[5] = regCount & 0xFF;
            frame_length = 6;
            break;
        }
        case MB_QUERY_WRITE_COILS:
        {
            DEBUG_LOGE(DEBUG_TAG_COMM,"MB", "Write coils NYI");
            while(1);
            // Build the frame for writing coils
            if(regCount == 1)
                frame[1] = MB_FUNC_WRITE_SINGLE_COIL;
            else
                frame[1] = MB_FUNC_WRITE_MULTIPLE_COILS;
            frame[2] = (address >> 8) & 0xFF;
            frame[3] = address & 0xFF;
            frame[4] = (regCount >> 8) & 0xFF;
            frame[5] = regCount & 0xFF;
            frame_length = 6;
            break;
        }
        case MB_QUERY_READ_DISCRETE_INPUTS:
        {
            DEBUG_LOGE(DEBUG_TAG_COMM,"MB", "Read discrete inputs NYI");
            vTaskSuspend(NULL);
            // Build the frame for reading discrete inputs
            frame[1] = MB_FUNC_READ_DISCRETE_INPUTS;
            frame[2] = (address >> 8) & 0xFF;
            frame[3] = address & 0xFF;
            frame[4] = (regCount >> 8) & 0xFF;
            frame[5] = regCount & 0xFF;
            frame_length = 6;
            break;
        }
        case MB_QUERY_READ_INPUT_REGISTERS:
        {
            // Build the frame for reading input registers
            frame[1] = MB_FUNC_READ_INPUT_REGISTERS;
            frame[2] = (address >> 8) & 0xFF;
            frame[3] = address & 0xFF;
            frame[4] = (regCount >> 8) & 0xFF;
            frame[5] = regCount & 0xFF;
            frame_length = 6;
            break;
        }
        case MB_QUERY_READ_HOLDING_REGISTERS:
        {
            // Build the frame for reading holding registers
            frame[1] = MB_FUNC_READ_HOLDING_REGISTERS;
            frame[2] = (address >> 8) & 0xFF;
            frame[3] = address & 0xFF;
            frame[4] = (regCount >> 8) & 0xFF;
            frame[5] = regCount & 0xFF;
            frame_length = 6;
            break;
        }
        case MB_QUERY_WRITE_HOLDING_REGISTERS:
        {   
            // Build the frame for writing holding registers
            if(regCount == 1)
                frame[1] = MB_FUNC_WRITE_SINGLE_REGISTER;
            else
                frame[1] = MB_FUNC_WRITE_MULTIPLE_REGISTERS;

            frame[2] = (address >> 8) & 0xFF;
            frame[3] = address & 0xFF;
            frame[4] = (regCount >> 8) & 0xFF;
            frame[5] = regCount & 0xFF;
            frame[6] = regCount * 2;

            // Copy the registers data into frame
            mb_regRead(slave, queryType, address, regCount, &frame[7]);
            frame_length = 7 + (regCount * 2); // Since each register is 2 bytes
            break;
        }
    
        default:
            return; // Unsupported query type
    }
    
    // Append CRC16
    uint16_t crc = mbframeCalculateCRC16(frame, frame_length);
    frame[frame_length++] = crc & 0xFF;
    frame[frame_length++] = (crc >> 8) & 0xFF;
    port->tx_buffer_length = frame_length;
}

modbus_error_t mbRxFrameParse(modbus_port_t *port, uint8_t slaveId, modbus_func_code_t funcCode, uint16_t address, uint16_t regCount)
{
    DEBUG_ASSERT(port != NULL);
        
    uint16_t length = port->rx_buffer_length;
    // Check the frame length
    if((length < 5) || (length > MODBUS_FRAME_MAX_LENGTH))
        return MB_ERROR_INVALID_FRAME;

    // char hex[20];
	// if (DEBUG_LogLevelGet() >= DEBUG_LEVEL_DEBUG)
	// {
	// 	sprintf(hex,"Rx[%d]> ",length);
	// 	DEBUG_LOGD(DEBUG_TAG_MODBUS,"MB",hex);
	// 	for(uint16_t i=0; i<length ; i++ )

	// 	{
	// 		sprintf(hex,"%.2X  ",frame[i]); 		DEBUG_LOG_RAW(hex);
	// 	}
	// }
    uint8_t *frame = port->rx_buffer;  // A valid buffer as it comes from port struct

    // Check the CRC
    uint16_t crc = mbframeCalculateCRC16(frame, length - 2);
    if (crc != ((frame[length - 1] << 8) | frame[length - 2]))
        return MB_ERROR_CRC_MISMATCH;

    // Extract the slave ID and function code
    if(slaveId != frame[0])
        return MB_ERROR_INVALID_SLAVE;
    
    // Find the corresponding slave
    modbus_slave_info_t *slave = mbGetSlaveInfo(port, slaveId);
    if (slave == NULL)
        return MB_ERROR_INVALID_SLAVE;

    slave->status.connected = 1; // Mark the slave as connected

    if(funcCode != frame[1])
        return MB_ERROR_INVALID_FUNCTION_CODE;

    // Process the frame based on the function code
    switch (funcCode)
    {
        case MB_FUNC_READ_COILS:
        {
            // Handle read coils response
            break;
        }
        case MB_FUNC_WRITE_SINGLE_COIL:
        case MB_FUNC_WRITE_MULTIPLE_COILS:
            // Handle write coil response
            break;
        case MB_FUNC_READ_DISCRETE_INPUTS:
            // Handle read discrete inputs response
            break;
        case MB_FUNC_READ_INPUT_REGISTERS:
            // Handle read input registers response
            break;
        case MB_FUNC_READ_HOLDING_REGISTERS:
        {
            // DEBUG_LOGI(DEBUG_TAG_MODBUS, "MB", "Read Holding Registers ");
            mb_regWrite(slave, 
                MB_QUERY_READ_HOLDING_REGISTERS,
                address, 
                regCount, 
                &frame[3]);
            break;
        }
        case MB_FUNC_WRITE_SINGLE_REGISTER:
        case MB_FUNC_WRITE_MULTIPLE_REGISTERS:
        {   
            // DEBUG_LOGI(DEBUG_TAG_MODBUS, "MB", "Write Holding Registers ");
            // Handle write register response
            break;
        }
        default:
            return MB_ERROR_INVALID_FUNCTION_CODE;
    }
    return MB_ERROR_NONE;
}

uint16_t mbframeCalculateCRC16(uint8_t *data, uint16_t size)
{
    uint16_t crc = 0xFFFF;
    for (uint16_t i = 0; i < size; i++)
    {
        crc ^= data[i];
        for (uint8_t j = 0; j < 8; j++)
        {
            if (crc & 0x0001)
                crc = (crc >> 1) ^ 0xA001;
            else
                crc >>= 1;
        }
    }
    return crc;
}
