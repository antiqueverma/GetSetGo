
#ifndef VERSION_H_
#define VERSION_H_

#include "gsg_base.h"

#define APP_NAME                    "HoAuHub"
#define SYS_SW_BUILD_DATE           __DATE__
#define SYS_SW_BUILD_TIME           __TIME__

#define SYS_VERSION_DATABASE        0

#define SYS_VERSION_APP_MAJOR       1
#define SYS_VERSION_APP_MINOR       2
#define SYS_VERSION_APP_PATCH       3

#define SYS_VERSION_BSP_MAJOR       1
#define SYS_VERSION_BSP_MINOR       2
#define SYS_VERSION_BSP_PATCH       3

#define SYS_VERSION_MSP_MAJOR       1
#define SYS_VERSION_MSP_MINOR       2
#define SYS_VERSION_MSP_PATCH       3

// GSG Framework Related
#define SYS_VERSION_LIB_MAJOR       1
#define SYS_VERSION_LIB_MINOR       2
#define SYS_VERSION_LIB_PATCH       3

// Part and Serial Numbers
#define SYS_HW_PART_NUMBER          "H1D2025MV"     
#define SYS_SW_PART_NUMBER          "S1D2025MV"
#define SYS_SERIAL_NUMBER           "H1S1D2025MV"

typedef enum{
    SYS_INFO_APP_VERSION,
    SYS_INFO_BSP_VERSION,
    SYS_INFO_MSP_VERSION,
    SYS_INFO_LIB_VERSION,
    SYS_INFO_DB_VERSION,
    SYS_INFO_APP_BUILD_DATE,
    SYS_INFO_APP_BUILD_TIME,
    SYS_INFO_HW_PART_NUMBER,
    SYS_INFO_SW_PART_NUMBER,
    SYS_INFO_SERIAL_NUMBER,
    SYS_INFO_APP_NAME
} sys_info_field_t;

#define SYS_STRING_FIELD_LEN       16
#define NVM_ADD_SYS_INFO_OFFSET     25



// static_assert((sizeof(APP_NAME) <= SYS_STRING_FIELD_LEN), "App name too long");

// static_assert((sizeof(SYS_HW_PART_NUMBER) <= SYS_STRING_FIELD_LEN), "Hardware part number too long");

// static_assert((sizeof(SYS_SW_PART_NUMBER) <= SYS_STRING_FIELD_LEN), "Software part number too long");

// static_assert((sizeof(SYS_SERIAL_NUMBER) <= SYS_STRING_FIELD_LEN), "Serial number too long");

// Struct for storing all system information
typedef struct {
    // Software Versions
    uint32_t appVersion;
    uint32_t bspVersion;
    uint32_t mspVersion;
    uint32_t libVersion;
    uint16_t dbVersion;

    // Software Build Info
    char appBuildDate[12];
    char appBuildTime[10];

    char hwPartNumber[SYS_STRING_FIELD_LEN + 1];
    char swPartNumber[SYS_STRING_FIELD_LEN + 1];
    char serialNumber[SYS_STRING_FIELD_LEN + 1];
    char appName[SYS_STRING_FIELD_LEN + 1];
} systemInfo_t;

extern systemInfo_t SYS_INFO;


#endif // VERSION_H_