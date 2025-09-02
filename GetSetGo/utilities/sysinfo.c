
#include "sysinfo.h"

systemInfo_t SYS_INFO = {
    .appVersion = (SYS_VERSION_APP_MAJOR << 16) | (SYS_VERSION_APP_MINOR << 8) | (SYS_VERSION_APP_PATCH),
    .bspVersion = (SYS_VERSION_BSP_MAJOR << 16) | (SYS_VERSION_BSP_MINOR << 8) | (SYS_VERSION_BSP_PATCH),
    .mspVersion = (SYS_VERSION_MSP_MAJOR << 16) | (SYS_VERSION_MSP_MINOR << 8) | (SYS_VERSION_MSP_PATCH),
    .libVersion = (SYS_VERSION_LIB_MAJOR << 16) | (SYS_VERSION_LIB_MINOR << 8) | (SYS_VERSION_LIB_PATCH),
    .dbVersion  = SYS_VERSION_DATABASE,

    .appName = APP_NAME,
    .appBuildDate = SYS_SW_BUILD_DATE,
    .appBuildTime = SYS_SW_BUILD_TIME,
    .hwPartNumber = SYS_HW_PART_NUMBER,
    .swPartNumber = SYS_SW_PART_NUMBER,
    .serialNumber = SYS_SERIAL_NUMBER,
};

void SysInfo_Init( void )
{
    uint16_t byteCtr = 0;

    //NVM_writeBytes(NVM_ADD_SYS_INFO_OFFSET + byteCtr,&SYS_INFO.appVersion,sizeof(SYS_INFO.appVersion));
    byteCtr = sizeof(SYS_INFO.appVersion);

    //NVM_writeBytes(NVM_ADD_SYS_INFO_OFFSET + byteCtr,&SYS_INFO.bspVersion,sizeof(SYS_INFO.bspVersion));
    byteCtr = sizeof(SYS_INFO.bspVersion);

    //NVM_writeBytes(NVM_ADD_SYS_INFO_OFFSET + byteCtr,&SYS_INFO.mspVersion,sizeof(SYS_INFO.mspVersion));
    byteCtr = sizeof(SYS_INFO.mspVersion);

    //NVM_writeBytes(NVM_ADD_SYS_INFO_OFFSET + byteCtr,&SYS_INFO.libVersion,sizeof(SYS_INFO.libVersion));
    byteCtr = sizeof(SYS_INFO.libVersion);

    //snprintf(SYS_INFO.appName, SYS_STRING_FIELD_LEN, "%s", APP_NAME);
    //NVM_writeBytes(NVM_ADD_SYS_INFO_OFFSET + byteCtr, &SYS_INFO.appName, sizeof(SYS_INFO.appName));
    byteCtr = sizeof(SYS_INFO.appName);

    //snprintf(SYS_INFO.appBuildDate, SYS_STRING_FIELD_LEN, "%s", SYS_SW_BUILD_DATE);
    //NVM_writeBytes(NVM_ADD_SYS_INFO_OFFSET + byteCtr, &SYS_INFO.appBuildDate, sizeof(SYS_INFO.appBuildDate));
    byteCtr = sizeof(SYS_INFO.appBuildDate);

    //snprintf(SYS_INFO.appBuildTime, SYS_STRING_FIELD_LEN, "%s", SYS_SW_BUILD_TIME);
    //NVM_writeBytes(NVM_ADD_SYS_INFO_OFFSET + byteCtr, &SYS_INFO.appBuildTime, sizeof(SYS_INFO.appBuildTime));
    byteCtr = sizeof(SYS_INFO.appBuildTime);

    //snprintf(SYS_INFO.hwPartNumber, SYS_STRING_FIELD_LEN, "%s", SYS_HW_PART_NUMBER);
    //NVM_writeBytes(NVM_ADD_SYS_INFO_OFFSET + byteCtr, &SYS_INFO.hwPartNumber, sizeof(SYS_INFO.hwPartNumber));
    byteCtr = sizeof(SYS_INFO.hwPartNumber);

    //snprintf(SYS_INFO.swPartNumber, SYS_STRING_FIELD_LEN, "%s", SYS_SW_PART_NUMBER);
    //NVM_writeBytes(NVM_ADD_SYS_INFO_OFFSET + byteCtr, &SYS_INFO.swPartNumber, sizeof(SYS_INFO.swPartNumber));
    byteCtr = sizeof(SYS_INFO.swPartNumber);

    //snprintf(SYS_INFO.serialNumber, SYS_STRING_FIELD_LEN, "%s", SYS_SERIAL_NUMBER);
    //NVM_writeBytes(NVM_ADD_SYS_INFO_OFFSET + byteCtr, &SYS_INFO.serialNumber, sizeof(SYS_INFO.serialNumber));
    byteCtr = sizeof(SYS_INFO.serialNumber);
}

