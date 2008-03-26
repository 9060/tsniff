#ifndef B_CAS_CARD_FX2STREAM_H
#define B_CAS_CARD_FX2STREAM_H

#include "portable.h"

typedef struct {
    void *data;
    uint_t len;
} B_CAS_CARD_CHUNK;

#ifdef __cplusplus
extern "C" {
#endif

extern GAsyncQueue *b_cas_fx2stream_get_async_queue(void);

#ifdef __cplusplus
}
#endif

#endif /* B_CAS_CARD_H */
