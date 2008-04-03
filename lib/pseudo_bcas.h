#ifndef PSEUDO_BCAS_H
#define PSEUDO_BCAS_H

#ifdef __cplusplus
extern "C" {
#endif

/*
 * PSEUDO_B_CAS_CARD *bcas = bcas_card_streaming_new();
 * b25->set_b_cas_card((B_CAS_CARD *)bcas);
 */
typedef struct PSEUDO_B_CAS_CARD {
    B_CAS_CARD super;

    void (*push)(void *bcas, guint8 *data, guint len);
    void (*set_queue_len)(void *bcas, guint len);
	void (*set_init_status)(void *bcas, const guint8 *system_key, const guint8 *init_cbc);
	gboolean (*set_init_status_from_hex)(void *bcas, const gchar *system_key, const gchar *init_cbc);
} PSEUDO_B_CAS_CARD;

PSEUDO_B_CAS_CARD *
pseudo_bcas_new(void);

#ifdef __cplusplus
}
#endif

#endif /* PSEUDO_BCAS_H */
