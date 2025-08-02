

#ifndef MODBUS_MASTER_H_
#define MODBUS_MASTER_H_

#include "gsg_base.h"

typedef enum{
    MB_QUERY_APERIODIC,
    MB_QUERY_PERIOD_100_MS,
    MB_QUERY_PERIOD_500_MS,
    MB_QUERY_PERIOD_1_S,
    MB_QUERY_PERIOD_5_S,
    MB_QUERY_PERIOD_10_S,
    MB_QUERY_PERIOD_30_S,
    MB_QUERY_PERIOD_1_MIN,
    MB_QUERY_PERIOD_5_MIN,
} mb_query_periodicity_t;

//typedef struct{
//    modbus_port_t *port;
//    uint8_t funcCode;
//    uint16_t address;
//    uint16_t regCount;
//    mb_query_periodicity_t periodicity;
//} mb_master_query_t;

#endif /* MODBUS_MASTER_H_ */
