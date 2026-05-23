#include "sv_add.h"


#define SVAR_SEGMENT_COUNT      5
#define SEGMENT1_SIZE           100

typedef enum segmentList_t{
    RAM_ONLY,
    EEPROM_ONLY,
    SHADOWED_EEPROM
} segmentList_t;