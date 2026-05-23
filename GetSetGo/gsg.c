#include "gsg_config.h"
#include "gsg_base.h"

void GSG_init(void)
{
    uint8_t initStatus = 0;
    #if (GSG_USE_DEBUG == GSG_ENABLE)
        DEBUG_Init();
    #endif

    #if (GSG_USE_NVM == GSG_ENABLE)
        NVM_Init();
    #endif
}