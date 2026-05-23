

#ifndef SVAR_H_
#define SVAR_H_
#if (defined(GSG_USE_SVAR) && (GSG_USE_SVAR == GSG_ENABLE))
#include "svar_internal.h"

gsg_result_t SVAR_registerModule(svar_module_t *module);
gsg_result_t SVAR_Set(uint32_t id, void *data);
gsg_result_t SVAR_Get(uint32_t id, void *data);

gsg_result_t SVAR_SetMin(uint32_t id, void *data);
gsg_result_t SVAR_SetMax(uint32_t id, void *data);
gsg_result_t SVAR_SetDefault(uint32_t id, void *data);

gsg_result_t SVAR_GetMin(uint32_t id, void *data);
gsg_result_t SVAR_GetMax(uint32_t id, void *data);
gsg_result_t SVAR_GetDefault(uint32_t id, void *data);

gsg_result_t SVAR_registerSetCallback(uint32_t id, svar_set_callback_t cb);
gsg_result_t SVAR_registerGetCallback(uint32_t id, svar_get_callback_t cb);

#endif
#endif /* SVAR_H_ */
