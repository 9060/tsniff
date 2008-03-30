#ifndef B_CAS_CARD_FX2STREAM_H
#define B_CAS_CARD_FX2STREAM_H

#ifdef __cplusplus
extern "C" {
#endif

B_CAS_CARD *
bcas_card_streaming_new(void);

void
bcas_card_streaming_set_queue_len(B_CAS_CARD *bcas, guint len);

void
bcas_card_streaming_push(B_CAS_CARD *, guint8 *data, guint len);

#ifdef __cplusplus
}
#endif

#endif /* B_CAS_CARD_H */
