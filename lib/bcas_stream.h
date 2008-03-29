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
	(((p)->header == 0x0040) && ((p)->len == 0x18))


typedef void (*BCASStreamCallbackFunc)(const BCASPacket *packet);


BCASStream *
bcas_stream_init(void);

void
bcas_stream_free(BCASStream *self);

void
bcas_stream_push(BCASStream *self, guint8 *data, uint len, BCASStreamCallbackFunc *cbfn);

#endif	/* BCAS_STREAM_H_INCLUDED */
