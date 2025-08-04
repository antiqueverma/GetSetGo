#include "modbus.h"

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
    // Set the frame length
    port->tx_buffer_length = frame_length + 2; // Add 2 for CRC
    // CRC will be appended by PHY layer
}
