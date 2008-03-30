#ifndef BCAS_STREAM_H_INCLUDED
#define BCAS_STREAM_H_INCLUDED

typedef struct BCASPacket {
	guint16 header;
	guint8 len;
	guint8 payload[G_MAXUINT8];
} BCASPacket;

struct BCASStream;
typedef struct BCASStream BCASStream;

/**
 * BCASパケット \p が ECM Request パケットであれば TRUE を返します。
 * @param p	BCASPacket
 */
#define BCAS_IS_ECM_REQUEST_PACKET(p)			\
	(((p)->payload[0] == 0x90) &&				\
	 ((p)->payload[1] == 0x34) &&				\
	 ((p)->payload[2] == 0x00) &&				\
	 ((p)->payload[3] == 0x00))

/**
 * BCASパケット \p が ECM Response パケットであれば TRUE を返します。
 * @param p	BCASPacket
 */
#define BCAS_IS_ECM_RESPONSE_PACKET(p)				\
	(((p)->len == 0x19) && ((p)->payload[0] == 0x00) && ((p)->payload[1] == 0x15))

#define BCAS_ECM_PACKET_DATA_LEN_INDEX 4
#define BCAS_ECM_PACKET_DATA_INDEX 5
#define BCAS_ECM_PACKET_FLAGS_INDEX 4
#define BCAS_ECM_PACKET_KEY_INDEX 6
#define BCAS_ECM_PACKET_KEY_SIZE 16

/**
 * @param packet
 * @param is_first_sync	同期後、最初に現われたパケットであれば TRUE
 * @param user_data
 */
typedef void (*BCASStreamCallbackFunc)(const BCASPacket *packet, gboolean is_first_sync, gpointer user_data);


BCASStream *
bcas_stream_new(void);

void
bcas_stream_free(BCASStream *self);

void
bcas_stream_push(BCASStream *self, guint8 *data, guint len, BCASStreamCallbackFunc cbfn, gpointer user_data);

#endif	/* BCAS_STREAM_H_INCLUDED */
