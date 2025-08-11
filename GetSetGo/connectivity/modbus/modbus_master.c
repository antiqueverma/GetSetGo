
#include "modbus_master.h"


// create master event group


// gsg_result_t MB_masterRegisterQuery(modbus_port_t *port, const mb_master_query_t *query)
// {
//     for (uint8_t i = 0; i < MODBUS_MASTER_QUERY_LIST_LENGTH; i++)
//     {
//         // check for a free slot in the list and save the query
//         if (port->queryList[i] == NULL)
//         {
//             port->queryList[i] = query;
//             return GSG_SUCCESS;
//         }
//     }
//     return GSG_OVERFLOW; // No space
// }

// gsg_result_t MB_masterUnregisterQuery(modbus_port_t *port, const mb_master_query_t *query)
// {
//     for (uint8_t i = 0; i < MODBUS_MASTER_QUERY_LIST_LENGTH; i++)
//     {
//         // find the query in the list and remove it
//         if (port->queryList[i] == query)
//         {
//             port->queryList[i] = NULL;
//             return GSG_SUCCESS;
//         }
//     }
//     return GSG_NOT_FOUND; // Query not found
// }

void mbMasterQueryTimerHandler(void *arg)
{
    modbus_port_t *port = (modbus_port_t *)arg;
    if (port == NULL)
        return;

    uint32_t currentTime = osKernelGetTickCount();  // Get current time in ms

    for (uint8_t i = 0; i < port->queryTableLength ; i++)
    {
        mb_master_query_t *query = &port->queryTable[i];
        
        if (query == NULL)
            continue;
        
        uint32_t elapsed = currentTime - query->lastExecutionTimeMs;
        uint32_t periodMs = 0;

        switch (query->periodicity)
        {
            case MB_MASTER_QUERY_APERIODIC:     periodMs = 0;       break;
            case MB_MASTER_QUERY_PERIOD_100_MS: periodMs = 100;     break;
            case MB_MASTER_QUERY_PERIOD_200_MS: periodMs = 200;     break;
            case MB_MASTER_QUERY_PERIOD_500_MS: periodMs = 500;     break;
            case MB_MASTER_QUERY_PERIOD_1_S:    periodMs = 1000;    break;
            case MB_MASTER_QUERY_PERIOD_5_S:    periodMs = 5000;    break;
            case MB_MASTER_QUERY_PERIOD_10_S:   periodMs = 10000;   break;
            case MB_MASTER_QUERY_PERIOD_30_S:   periodMs = 30000;   break;
            case MB_MASTER_QUERY_PERIOD_1_MIN:  periodMs = 60000;   break;
            case MB_MASTER_QUERY_PERIOD_5_MIN:  periodMs = 300000;  break;
            case MB_MASTER_QUERY_PERIOD_10_MIN: periodMs = 600000;  break;
            case MB_MASTER_QUERY_PERIOD_30_MIN: periodMs = 1800000; break;
            case MB_MASTER_QUERY_PERIOD_1_HOUR: periodMs = 3600000; break;
            case MB_MASTER_QUERY_DISABLED:  // Skip disabled queries
            default:  // Skip unrecognized types
                continue;              
        }

        if ((elapsed >= periodMs) || (periodMs == 0))
        {
            query->lastExecutionTimeMs = currentTime;
            if (osMessageQueuePut(port->queryQueueHandle, &query, 0, 0) != osOK)
            {
                DEBUG_LOGW(DEBUG_TAG_MODBUS,"MB","Query queue full");
            }
            osEventFlagsSet(port->eventHandle, MODBUS_EVENT_QUERY_TODO);// also set the flag

            if(query->periodicity == MB_MASTER_QUERY_APERIODIC)
                query->periodicity = MB_MASTER_QUERY_DISABLED;
        }
    }
}


//            char hex[10];
//            sprintf(hex, "Q[%d]", i);
//            DEBUG_LOGI(DEBUG_TAG_MODBUS,"MB",hex);
            // Convert periodicity enum to milliseconds