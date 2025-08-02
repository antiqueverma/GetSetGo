

#ifndef CONFIG_PARSER_H_
#define CONFIG_PARSER_H_

#include "gsg_base.h"

typedef struct {
    char key[64];
    char value[128];
} config_entry_t;

typedef struct {
    config_entry_t entries[32];
    uint8_t entry_count;
} config_t;

typedef enum
{
    CONFIG_ID_CLI_AUTH,
    CONFIG_ID_BAUDRATE,
    CONFIG_ID_FEATURE_ENABLE,
    CONFIG_ID_THEME,
    CONFIG_COUNT
} config_id_t;

typedef struct
{
    uint32_t key;
    uint8_t type;   // e.g., 1 = int, 2 = bool, 3 = blob
    union {
        int32_t i;
        uint8_t b;
        uint8_t raw[8];
    } value;
} config_entry_t;

#endif // CONFIG_PARSER_H_