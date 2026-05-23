
#ifndef SVAR_API_H_
#define SVAR_API_H_
#include "gsg_config.h"
#ifdef GSG_USE_SVAR
#if (GSG_USE_SVAR == GSG_ENABLE)

// Setter Functions
gsg_result_t SVAR_setMin(uint32_t id, void *value);
gsg_result_t SVAR_setMax(uint32_t id, void *value);
gsg_result_t SVAR_setDefault(uint32_t id, void *value);
gsg_result_t SVAR_setValue(uint32_t id, void *value);
gsg_result_t SVAR_setName(uint32_t id, const char *name);
// Getter Functions
gsg_result_t SVAR_getMin(uint32_t id, void *value);
gsg_result_t SVAR_getMax(uint32_t id, void *value);
gsg_result_t SVAR_getDefault(uint32_t id, void *value);
gsg_result_t SVAR_getValue(uint32_t id, void *value);
gsg_result_t SVAR_getName(uint32_t id, char *nameBuffer, size_t bufferSize);    

#endif
#endif
#endif /* SVAR_API_H_ */