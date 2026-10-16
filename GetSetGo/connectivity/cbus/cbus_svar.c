#include <string.h>
#include "gsg_base.h"
#include "cbus.h"
#include "301/CO_ODinterface.h"
#include "OD.h"
#include "services/svar/svar.h"

#if (CBUS_USE_SVAR_DATABASE == 1)

#define CBUS_SVAR_FIRST_OD_INDEX    0x2000  // Communication profile objects (< 0x2000) stay in the OD

static ODR_t cbusSvarRead(OD_stream_t *stream, void *buf, OD_size_t count, OD_size_t *countRead);
static ODR_t cbusSvarWrite(OD_stream_t *stream, const void *buf, OD_size_t count, OD_size_t *countWritten);

// One extension shared by all OD entries; the stream index / sub-index select the SVAR
static OD_extension_t cbusSvarExt = {
    .object = NULL,
    .read = cbusSvarRead,
    .write = cbusSvarWrite,
};

static ODR_t cbusSvarResult(gsg_result_t res)
{
    switch (res)
    {
        case GSG_SUCCESS:     return ODR_OK;
        default:   return ODR_IDX_NOT_EXIST;
//        case GSG_NOT_FOUND:   return ODR_IDX_NOT_EXIST;
//        case GSG_INVALID_ARG: return ODR_INVALID_VALUE;
//        default:              return ODR_DEV_INCOMPAT;
    }
}

static bool_t cbusSvarLookup(OD_stream_t *stream, uint32_t *svarId)
{
    uint32_t canId = ((uint32_t)stream->index << 8) | stream->subIndex;
    return (SVAR_GetIdFromCanId(canId, svarId) == GSG_SUCCESS);
}

static ODR_t cbusSvarRead(OD_stream_t *stream, void *buf, OD_size_t count, OD_size_t *countRead)
{
    uint32_t svarId;
    if (!cbusSvarLookup(stream, &svarId))
    {
        return OD_readOriginal(stream, buf, count, countRead);  // Not mapped to SVAR, use OD
    }

    uint8_t tmp[8] = {0};   // SVAR_Get writes only the variable's native size
    ODR_t ret = cbusSvarResult(SVAR_Get(svarId, tmp));
    if (ret != ODR_OK)
    {
        return ret;
    }

    OD_size_t len = (stream->dataLength < sizeof(tmp)) ? stream->dataLength : sizeof(tmp);
    if (count < len)
    {
        return ODR_DEV_INCOMPAT;
    }
    memcpy(buf, tmp, len);
    *countRead = len;
    return ODR_OK;
}

static ODR_t cbusSvarWrite(OD_stream_t *stream, const void *buf, OD_size_t count, OD_size_t *countWritten)
{
    uint32_t svarId;
    if (!cbusSvarLookup(stream, &svarId))
    {
        return OD_writeOriginal(stream, buf, count, countWritten);  // Not mapped to SVAR, use OD
    }

    uint8_t tmp[8] = {0};
    if (count > sizeof(tmp))
    {
        return ODR_DATA_LONG;
    }
    memcpy(tmp, buf, count);

    ODR_t ret = cbusSvarResult(SVAR_Set(svarId, tmp));
    if (ret == ODR_OK)
    {
        *countWritten = count;
    }
    return ret;
}

void CBUS_SVAR_Init(void)
{
    for (uint16_t i = 0; i < OD->size; i++)
    {
        if (OD->list[i].index >= CBUS_SVAR_FIRST_OD_INDEX)
        {
            OD_extension_init(&OD->list[i], &cbusSvarExt);
        }
    }
}

#else

void CBUS_SVAR_Init(void)
{
}

#endif
