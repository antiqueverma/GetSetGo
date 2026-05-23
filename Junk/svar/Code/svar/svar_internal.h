#include "sv_add.h"

typedef enum segmentFlavor_t{
    RAM_ONLY,
    EEPROM_ONLY,        // to removed. RAM should always be used 
    SHADOWED_EEPROM
} segmentFlavor_t;

typedef struct setupData_t{
    uint16_t    factoryBytes;
    uint16_t    pageSize;
    uint8_t     segCount;
    uint8_t     packedStorage_en;
} setupData_t;

typedef struct segmentData_t{
    const uint32_t  addOffset;      // Logical SVAR address (as per svar_add.h)
    uint32_t        nvmOffset;      // EEPROM offset address for the whole segment 
    uint32_t        writeTime;      // Write time interval to avoid continuous writing to eeprom
    uint8_t         *buffer;        //RAM buffer 
    storageFlavor_t flavor;
} segmentData_t;

typedef uint32_t svAdd_t

#define SVAR_FACTORY_BYTES_VALUE    (0xABCD)
#define ASSERT                      
extern setupData_t svarSetup;
extern segmentData_t segment[];