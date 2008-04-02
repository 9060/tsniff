#include <stdlib.h>
#include <string.h>
#include <glib.h>

#include "portable.h"
#include "b_cas_card.h"
#include "b_cas_card_error_code.h"
#include "bcas_stream.h"
#include "bcas_card_streaming.h"

static B_CAS_INIT_STATUS st_bcas_init_status = {
	{ 0x36,0x31,0x04,0x66,0x4B,0x17,0xEA,0x5C,0x32,0xDF,0x9C,0xF5,0xC4,0xC3,0x6C,0x1B,
	  0xEC,0x99,0x39,0x21,0x68,0x9D,0x4B,0xB7,0xB7,0x4E,0x40,0x84,0x0D,0x2E,0x7D,0x98, },
	{ 0xFE,0x27,0x19,0x99,0x19,0x69,0x09,0x11, },
	0,
	0,
};

typedef struct ECMPacket {
	/* Requested packet */
	guint8 len;
	guint8 data[G_MAXUINT8];

	/* Responed packet */
	guint16 flag;
	guint8 key[BCAS_ECM_PACKET_KEY_SIZE];
} ECMPacket;

typedef struct Context {
	BCASStream *stream;
	GQueue *ecm_queue;
	guint ecm_queue_len;

	ECMPacket *pending_ecm_packet;
	gint response_delay;
} Context;


static gchar *
dump_ecm_packet(ECMPacket *ecm)
{
	static gchar hexdump[2 * G_MAXUINT8 + 1];
	gint i;

	hexdump[0] = '\0';
	for (i = 0; i < ecm->len; ++i) {
		gchar x[3];
		g_sprintf(x, "%02x", ecm->data[i]);
		g_strlcat(hexdump, x, 2 * G_MAXUINT8);
	}
	return hexdump;
}

static void
parse_packet(const BCASPacket *packet, gboolean is_first_sync, gpointer user_data)
{
	Context *self = (Context *)user_data;

	/* ECM Response パケット待ちであれば破棄する */
	if (is_first_sync && self->pending_ecm_packet) {
		g_warning("[bcas_card_streaming] dispose pending ECM request packet");
		g_slice_free(ECMPacket, self->pending_ecm_packet);
		self->pending_ecm_packet = NULL;
	}

	++self->response_delay;

	if (BCAS_IS_ECM_REQUEST_PACKET(packet)) {
		/* ECM Request パケットであれば対応する ECM Response を待つ */
		/* 既に Response を待っている? */
		if (self->pending_ecm_packet) {
			g_warning("[bcas_card_streaming] found a new ECM request before completing old request (%d packets delta)",
					  self->response_delay - 1);
		} else {
			self->pending_ecm_packet = g_slice_new(ECMPacket);
		}
		self->pending_ecm_packet->len = packet->payload[BCAS_ECM_PACKET_DATA_LEN_INDEX];
		memcpy(self->pending_ecm_packet->data, &packet->payload[BCAS_ECM_PACKET_DATA_INDEX], self->pending_ecm_packet->len);

		self->response_delay = 0;
	} else if (BCAS_IS_ECM_RESPONSE_PACKET(packet)) {
		/* Request がないのに Response が来た */
		if (!self->pending_ecm_packet) {
			g_warning("[bcas_card_streaming] not requested ECM response found");
		} else {
			if (self->response_delay > 1) {
				g_warning("[bcas_card_streaming] ECM response delayed by %d packets, maybe incorrect",
						  self->response_delay - 1);
			}
			self->pending_ecm_packet->flag = 
				(packet->payload[BCAS_ECM_PACKET_FLAGS_INDEX] << 8) | packet->payload[BCAS_ECM_PACKET_FLAGS_INDEX + 1];
			memcpy(self->pending_ecm_packet->key, &packet->payload[BCAS_ECM_PACKET_KEY_INDEX], BCAS_ECM_PACKET_KEY_SIZE);

			/* ECMキューに追加 */
			g_debug("[bcas_card_streaming] ECM REGIST [%s]", dump_ecm_packet(self->pending_ecm_packet));
			g_queue_push_tail(self->ecm_queue, self->pending_ecm_packet);
			if (self->ecm_queue->length > self->ecm_queue_len) {
				/* ECMキューの最大長を越えていたら、最も古い ECM を捨てる */
				g_slice_free(ECMPacket, g_queue_pop_head(self->ecm_queue));
			}

			self->pending_ecm_packet = NULL;
		}
	}
}

static gint
compare_ecm_packet(gconstpointer plhs, gconstpointer prhs)
{
	const ECMPacket *lhs = (const ECMPacket *)plhs;
	const ECMPacket *rhs = (const ECMPacket *)prhs;

	if (lhs->len == rhs->len) {
		return memcmp(lhs->data, rhs->data, lhs->len);
	} else {
		return rhs->len - lhs->len;
	}
}


/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 interface method implementation
 ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/
static int
init_b_cas_card(void *bcas)
{
	Context *self = g_new(Context, 1);
	((B_CAS_CARD *)bcas)->private_data = self;
	self->ecm_queue = g_queue_new();
	self->ecm_queue_len = 128;
	self->stream = bcas_stream_new();
	self->pending_ecm_packet = NULL;
	self->response_delay = 0;

	return 0;
}

static void
release_b_cas_card(void *bcas)
{
	Context *self = (Context *)((B_CAS_CARD *)bcas)->private_data;
	gpointer ecm;

	bcas_stream_free(self->stream);

	while (ecm = g_queue_pop_head(self->ecm_queue)) {
		g_free(ecm);
	}
	g_free(self->ecm_queue);

	g_free(bcas);
}

static int get_init_status_b_cas_card(void *bcas, B_CAS_INIT_STATUS *stat)
{
	memcpy(stat, &st_bcas_init_status, sizeof(B_CAS_INIT_STATUS));
	return 0;
}

static int proc_ecm_b_cas_card(void *bcas, B_CAS_ECM_RESULT *dst, uint8_t *src, int len)
{
	Context *self = (Context *)((B_CAS_CARD *)bcas)->private_data;
	gpointer data;
	ECMPacket src_packet;
	GList *match;
	ECMPacket *ecm = NULL;

	/* ECMキューからECMパケットを検索 */
	src_packet.len = len;
	memcpy(src_packet.data, src, len);

	match = g_queue_find_custom(self->ecm_queue, &src_packet, compare_ecm_packet);
	if (match) {
		ecm = match->data;
	}

	if (ecm) {
		memcpy(dst->scramble_key, ecm->key, BCAS_ECM_PACKET_KEY_SIZE);
		dst->return_code = ecm->flag;
		g_debug("[bcas_card_streaming] ECM FOUND  [%s]", dump_ecm_packet(ecm));
	} else {
		g_debug("[bcas_card_streaming] ECM FAILED [%s]", dump_ecm_packet(&src_packet));
		return -1;
	}

	return 0;
}


/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 global function implementation
 ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/
B_CAS_CARD *
bcas_card_streaming_new(void)
{
	B_CAS_CARD *r;

	r = g_new(B_CAS_CARD, 1);

	r->release = release_b_cas_card;
	r->init = init_b_cas_card;
	r->get_init_status = get_init_status_b_cas_card;
	r->proc_ecm = proc_ecm_b_cas_card;

	return r;
}

void
bcas_card_streaming_set_queue_len(B_CAS_CARD *bcas, guint len)
{
	Context *self = (Context *)((B_CAS_CARD *)bcas)->private_data;
	self->ecm_queue_len = len;
}

void
bcas_card_streaming_push(B_CAS_CARD *bcas, guint8 *data, guint len)
{
	Context *self = (Context *)((B_CAS_CARD *)bcas)->private_data;
	bcas_stream_push(self->stream, data, len, parse_packet, self);
}
