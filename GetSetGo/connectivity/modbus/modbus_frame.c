#include "modbus.h"
void mb_getCRC( uint8_t *frame, size_t frame_length, uint16_t *crc);


modbus_error_t mb_process_rx_frame(modbus_port_t *port, size_t frame_length)
{
    if (port == NULL || frame_length == 0) 
    {
        return MB_ERROR_PORT_NOT_FOUND; // Invalid parameters
    }

    // Check if the port is enabled and in the correct state to process a frame
    if (port->state != MB_PORT_STATE_RX_WAITING && port->state != MB_PORT_STATE_RX_PROCESSING) 
    {
        return MB_ERROR_NONE; // Port is not ready to process a frame
    }

    uint8_t *frame = port->rx_buffer;

    // Check CRC
    uint16_t calcCRC;
    uint16_t rxCRC;
    mb_getCRC(frame, frame_length, &calcCRC);
    rxCRC = (frame[frame_length - 2] << 8) | frame[frame_length - 1]; // Get the received CRC
    if (calcCRC != rxCRC) 
        return MB_ERROR_CRC_MISMATCH; // CRC mismatch, frame is invalid    

    // Set port state to processing
    port->state = MB_PORT_STATE_RX_PROCESSING;

    // Get the start address and number of registers from the frame
    uint16_t startAddress = (frame[2] << 8) | frame[3]; // Assuming start address is at index 2 and 3
    uint16_t numRegisters = (frame[4] << 8) | frame[5]; // Assuming number of registers is at index 4 and 5
    
    // Get the modbus function code from the frame
    uint8_t functionCode = frame[1]; // Assuming the function code is at index
    switch(functionCode)
    {
        case MB_FUNC_WRITE_MULTIPLE_REGISTERS:
        {
            // check if the port has holding registers
            if (port->holdingRegisters == NULL)
            {
                return MB_ERROR_INVALID_REGISTER; // No holding registers available
            }

            break;
        }
        case MB_FUNC_READ_HOLDING_REGISTERS:
        {
            // check if the port has holding registers
            if (port->holdingRegisters == NULL)
            {
                return MB_ERROR_INVALID_REGISTER; // No holding registers available
            }
            // Handle reading holding registers
            // This would typically involve accessing the holdingRegisters pointer in the port structure
            // and preparing a response frame with the requested register values.
            break;
        }
        default:
        {
            // Handle other function codes as needed
            // For example, you might want to log an error or send a response indicating an unsupported function.
            return MB_ERROR_INVALID_FUNCTION_CODE; // Unsupported function code
        }
    }

    // Prepare the response frame
    uint8_t txBuffCtr = 0;
    mb_tx_set_data
}

void mb_getCRC( uint8_t *frame, size_t frame_length, uint16_t *crc)
{
    if (frame == NULL || crc == NULL || frame_length < 2) {
        return; // Invalid parameters
    }

    // Calculate CRC for the given frame
    *crc = 0xFFFF; // Initial value

    for (size_t i = 0; i < frame_length - 2; i++) 
    {
        *crc ^= frame[i];
        for (int j = 0; j < 8; j++) 
        {
            if (*crc & 0x0001) 
            {
                *crc >>= 1;
                *crc ^= 0xA001; // Polynomial
            } 
            else 
            {
                *crc >>= 1;
            }
        }
    }
}

void mb_tx_set_data(modbus_port_t *port, uint8_t index, uint8_t *data, size_t data_length)
{
    if (port == NULL || data == NULL || data_length == 0) 
    {
        return; // Invalid parameters
    }

    // Check if the port is enabled and in the correct state to transmit data
    if (port->state != MB_PORT_STATE_TX_READY) 
    {
        return; // Port is not ready to transmit data
    }

    // Copy the data to the transmit buffer
    if (data_length > MB_MAX_FRAME_SIZE) 
    {
        return; // Data length exceeds maximum frame size
    }

    // Copy data to the transmit buffer
    for (uint8_t i = 0; i < data_length; i++)    
    {
        port->tx_buffer[index + i] = data[i];
    }
}

void mb_tx_set_crc(modbus_port_t *port)
{
    if (port == NULL) {
        return; // Invalid parameters
    }

    // Calculate and set the CRC for the transmit buffer
    uint16_t crc;
    mb_getCRC(port->tx_buffer, port->tx_buffer_length, &crc);
    port->tx_buffer[port->tx_buffer_length] = (crc >> 8) & 0xFF; // Set high byte
    port->tx_buffer[port->tx_buffer_length + 1] = crc & 0xFF;      // Set low byte
}

void mbTxGenFrame(modbus_port_t *port, mb_query_type_t queryType, uint8_t slaveId, uint16_t address, uint16_t regCount)
{
    if (port == NULL) {
        return; // Invalid parameters
    }

    // Prepare the Modbus frame
    uint8_t *frame = port->tx_buffer;
    size_t frame_length = 0;

    // Add the Modbus PDU (Protocol Data Unit) based on the query type
    frame[0] = slaveId;

    switch (queryType)
    {
        case MB_QUERY_READ_COILS:
        {
            DEBUG_LOGE(DEBUG_TAG_COMM,"MB", "Read coils NYI");
            while(1);
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
            frame[1] = MB_FUNC_WRITE_COILS;
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
            while(1);
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
            DEBUG_LOGE(DEBUG_TAG_COMM,"MB", "Read input registers NYI");
            while(1);
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
            frame_length = 6;
            break;
        }
    
        default:
            return; // Unsupported query type
    }

    // CRC will be appended by PHY layer
    mbPhySendData(port, frame, frame_length);
}
