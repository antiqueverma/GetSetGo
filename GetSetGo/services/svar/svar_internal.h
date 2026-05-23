
#ifndef SVAR_INTERNAL_H_
#define SVAR_INTERNAL_H_
#include <stdint.h>
#include <stdbool.h>

#include "FreeRTOS.h"
#include "semphr.h"
#include "gsg_defs.h"
#include "sys/nvm/nvm.h"

#include "gsg_config.h"

#ifndef GSG_CONFIG_SVAR_MAX_MODULES
#define SVAR_MAX_MODULES 5
#warning "GSG_CONFIG_SVAR_MAX_MODULES is not defined, using default value of 5. Define GSG_CONFIG_SVAR_MAX_MODULES in gsg_config.h to override."
#else
#define SVAR_MAX_MODULES GSG_CONFIG_SVAR_MAX_MODULES
#endif

typedef enum {
    SVAR_TYPE_INT8,
    SVAR_TYPE_INT16,
    SVAR_TYPE_INT32,
    SVAR_TYPE_INT64,
    SVAR_TYPE_UINT8,
    SVAR_TYPE_UINT16,
    SVAR_TYPE_UINT32,
    SVAR_TYPE_UINT64,
    SVAR_TYPE_FLOAT,
    SVAR_TYPE_BOOL,
    SVAR_TYPE_CHAR,
    SVAR_TYPE_STRING,
    SVAR_TYPE_GROUP,
} svar_type_e;

typedef enum
{
    SVAR_CAT_NONE = 0,
    SVAR_CAT_CONFIG,
    SVAR_CAT_RUNTIME,
    SVAR_CAT_EVENT,      // future use
    SVAR_CAT_MAX
} svar_category_e;

typedef enum {
    SVAR_ACCESS_DISABLED = 0,
    SVAR_ACCESS_READ_ONLY,
    SVAR_ACCESS_READ_WRITE
} svar_access_t;

// typedef enum {
//     SVIF_APPCODE = 0,
//     SVIF_CLI,
//     SVIF_MODBUS,
//     SVIF_CAN,
//     __SVIF_MAX
// } svar_interface_type_t;

typedef struct 
{
    uint8_t        persistent:1;
    uint8_t        readOnly:1;
    uint8_t        minWritable:1;
    uint8_t        maxWritable:1;
    uint8_t        defWritable:1;

    uint8_t        __reservedBits:3; // Reserved bits for future use
} svar_flags_t;

typedef union
{
    int8_t     i8;
    uint8_t    u8;
    int16_t    i16;
    uint16_t   u16;
    int32_t    i32;
    uint32_t   u32;
    float      f;
    bool       b;
    char       c;
#if SVAR_ENABLE_64_BIT
    int64_t    i64;
    uint64_t   u64;
#endif
    char       *str;
} svar_value_t;


// Callback function type definitions
typedef gsg_result_t (*svar_set_callback_t)(void *value);
typedef gsg_result_t (*svar_get_callback_t)(void *value);

typedef uint8_t svar_type_t;
typedef uint8_t svar_category_t;

typedef struct {
    
    svar_set_callback_t      setCb;
    svar_get_callback_t      getCb;
    char                    *name;

    svar_value_t            value;
    svar_value_t            min;
    svar_value_t            max;
    svar_value_t            def;

    const uint32_t          id;
    const uint32_t          parent;
    const uint32_t          nvmAddr;
    
    // 16bits
    uint16_t if0Access:2;
    uint16_t if1Access:2;
    uint16_t if2Access:2;
    uint16_t if3Access:2;
    uint16_t if4Access:2;
    uint16_t if5Access:2;
    uint16_t if6Access:2;
    uint16_t if7Access:2;

    uint8_t                 type;
    uint8_t                 category;
    svar_flags_t            flags; 
    uint8_t                 __reserved;
} system_variable_t;

typedef struct {
    const system_variable_t *table;
    const uint16_t count;
    const uint16_t varOffset;
    nvm_device_t *nvmHandle;
} svar_module_t;

#endif /* SVAR_INTERNAL_H_ */
