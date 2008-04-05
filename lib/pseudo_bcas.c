#include <stdlib.h>
#include <string.h>
#include <glib.h>

#include "portable.h"
#include "b_cas_card.h"
#include "b_cas_card_error_code.h"
#include "bcas_stream.h"
#include "pseudo_bcas.h"

typedef struct ECMPacket {
	/* Requested packet */
	guint8 len;
	guint8 data[G_MAXUINT8];

	/* Responed packet */
	guint16 flag;
	guint8 key[BCAS_ECM_PACKET_KEY_SIZE];
	GTimeVal arrived_time;
} ECMPacket;

typedef struct Context {
	B_CAS_INIT_STATUS init_status;
	gboolean is_init_status_valid;

	BCASStream *stream;
	GQueue *ecm_queue;
	guint ecm_queue_len;

	ECMPacket *pending_ecm_packet;
	gint response_delay;
} Context;


static guint8 *
hexparse(const guint8 *hexdump, guint len)
{
	guint8 *data;
	guint i;

	if (hexdump == NULL) {
		g_warning("[pseudo_bcas] NULL pointer passed");
		return NULL;
	}
	if (len * 2 != strlen(hexdump)) {
		g_warning("[pseudo_bcas] invalid length (expect=%d, actual=%d)", len * 2, strlen(hexdump));
		return NULL;
	}

	data = g_malloc0(len);
	for (i = 0; i < len * 2; ++i) {
		gint b = g_ascii_xdigit_value(hexdump[i]);
		if (b < 0) {
			g_warning("[pseudo_bcas] an invalid char '%c' appeared", hexdump[i]);
			g_free(data);
			return NULL;
		}
		data[i / 2] |= b << ((i & 1) ? 0 : 4);
	}
	return data;
}

static GString *
hexdump(guint8 *data, guint len, gboolean is_seperate)
{
	GString *result;
	guint i;

	result = g_string_sized_new(len * 3);
	for (i = 0; i < len; ++i) {
		g_string_append_printf(result, "%s%02x", ((i == 0 || !is_seperate) ? "" : " "), data[i]);
	}
	return result;
}

static void
parse_packet(const BCASPacket *packet, gboolean is_first_sync, gpointer user_data)
{
	Context *self = (Context *)user_data;

	/* ECM Response パケット待ちであれば破棄する */
	if (is_first_sync && self->pending_ecm_packet) {
		g_warning("[pseudo_bcas] dispose pending ECM request packet");
		g_slice_free(ECMPacket, self->pending_ecm_packet);
		self->pending_ecm_packet = NULL;
	}

	++self->response_delay;

	if (BCAS_IS_ECM_REQUEST_PACKET(packet)) {
		/* ECM Request パケットであれば対応する ECM Response を待つ */
		/* 既に Response を待っている? */
		if (self->pending_ecm_packet) {
			g_warning("[pseudo_bcas] found a new ECM request before completing old request (%d packets delta)",
					  self->response_delay);
		} else {
			self->pending_ecm_packet = g_slice_new(ECMPacket);
		}
		self->pending_ecm_packet->len = packet->payload[BCAS_ECM_PACKET_DATA_LEN_INDEX];
		memcpy(self->pending_ecm_packet->data, &packet->payload[BCAS_ECM_PACKET_DATA_INDEX], self->pending_ecm_packet->len);

		self->response_delay = 0;
	} else if (BCAS_IS_ECM_RESPONSE_PACKET(packet)) {
		/* Request がないのに Response が来た */
		if (!self->pending_ecm_packet) {
			g_warning("[pseudo_bcas] not requested ECM response found");
		} else {
			GString *ecm_dump;
			if (self->response_delay > 1) {
				g_warning("[pseudo_bcas] ECM response delayed by %d packets, maybe incorrect",
						  self->response_delay);
			}
			self->pending_ecm_packet->flag = 
				(packet->payload[BCAS_ECM_PACKET_FLAGS_INDEX] << 8) | packet->payload[BCAS_ECM_PACKET_FLAGS_INDEX + 1];
			memcpy(self->pending_ecm_packet->key, &packet->payload[BCAS_ECM_PACKET_KEY_INDEX], BCAS_ECM_PACKET_KEY_SIZE);
			g_get_current_time(&self->pending_ecm_packet->arrived_time);

			/* ECMキューに追加 */
			ecm_dump = hexdump(self->pending_ecm_packet->data, self->pending_ecm_packet->len, FALSE);
			g_debug("[pseudo_bcas] ECM regist  [%s]", ecm_dump->str);
			g_string_free(ecm_dump, TRUE);

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
	Context *self;

	g_message("[pseudo_bcas] initialize");
	self = g_new(Context, 1);
	((B_CAS_CARD *)bcas)->private_data = self;
	memset(&self->init_status, 0, sizeof(self->init_status));
	self->is_init_status_valid = FALSE;
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

	g_message("[pseudo_bcas] release");

	bcas_stream_free(self->stream);

	while (ecm = g_queue_pop_head(self->ecm_queue)) {
		g_slice_free(ECMPacket, ecm);
	}
	g_queue_free(self->ecm_queue);

	g_free(bcas);
}

static int get_init_status_b_cas_card(void *bcas, B_CAS_INIT_STATUS *stat)
{
	Context *self = (Context *)((B_CAS_CARD *)bcas)->private_data;

	g_message("[pseudo_bcas] get initial status");

	if (self->is_init_status_valid) {
		memcpy(stat, &self->init_status, sizeof(B_CAS_INIT_STATUS));
		return 0;
	} else {
		g_warning("[pseudo_bcas] couldn't get initial status");
		return -1;
	}
}

static int proc_ecm_b_cas_card(void *bcas, B_CAS_ECM_RESULT *dst, uint8_t *src, int len)
{
	Context *self = (Context *)((B_CAS_CARD *)bcas)->private_data;
	gpointer data;
	ECMPacket src_packet;
	GList *match;
	ECMPacket *ecm = NULL;
	GString *ecm_dump;

	/* ECMキューからECMパケットを検索 */
	src_packet.len = len;
	memcpy(src_packet.data, src, len);

	match = g_queue_find_custom(self->ecm_queue, &src_packet, compare_ecm_packet);
	if (match) {
		ecm = match->data;
	}

	if (ecm) {
		GTimeVal now;
		gdouble diff;

		memcpy(dst->scramble_key, ecm->key, BCAS_ECM_PACKET_KEY_SIZE);
		dst->return_code = ecm->flag;

		g_get_current_time(&now);
		diff = (now.tv_sec - ecm->arrived_time.tv_sec) + (((gdouble)now.tv_usec - ecm->arrived_time.tv_usec) / 1000000);

		ecm_dump = hexdump(ecm->data, ecm->len, FALSE);
		g_debug("[pseudo_bcas] ECM found  [%s] (%+.3f)", ecm_dump->str, diff);
		g_string_free(ecm_dump, TRUE);
	} else {
		ecm_dump = hexdump(src_packet.data, src_packet.len, FALSE);
		g_warning("[pseudo_bcas] ECM FAILED [%s]", ecm_dump->str);
		g_string_free(ecm_dump, TRUE);
		return -1;
	}

	return 0;
}

static void
set_init_status(void *bcas, const guint8 *system_key, const guint8 *init_cbc)
{
	Context *self = (Context *)((B_CAS_CARD *)bcas)->private_data;

	if (system_key && init_cbc) {
		memcpy(&self->init_status.system_key, system_key, 32);
		memcpy(&self->init_status.init_cbc, init_cbc, 8);
		self->is_init_status_valid = TRUE;
	}
}

static gboolean
set_init_status_from_hex(void *bcas, const gchar *system_key, const gchar *init_cbc)
{
	Context *self = (Context *)((B_CAS_CARD *)bcas)->private_data;
	guint8 *syskey, *initcbc;
	gboolean result;

	syskey = hexparse(system_key, 32);
	initcbc = hexparse(init_cbc, 8);
	result = (syskey && initcbc) ? TRUE : FALSE;

	set_init_status(bcas, syskey, initcbc);

	if (syskey) g_free(syskey);
	if (initcbc) g_free(initcbc);

	return result;
}

static void
set_queue_len(void *bcas, guint len)
{
	Context *self = (Context *)((B_CAS_CARD *)bcas)->private_data;
	self->ecm_queue_len = len;
}

static gsize
get_queue_current_len(void *bcas)
{
	Context *self = (Context *)((B_CAS_CARD *)bcas)->private_data;
	return self->ecm_queue->length;
}

static void
push(void *bcas, guint8 *data, guint len)
{
	Context *self = (Context *)((B_CAS_CARD *)bcas)->private_data;
	bcas_stream_push(self->stream, data, len, parse_packet, self);
}

/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 global function implementation
 ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/
PSEUDO_B_CAS_CARD *
pseudo_bcas_new(void)
{
	PSEUDO_B_CAS_CARD *r;

	r = g_new(PSEUDO_B_CAS_CARD, 1);

	r->super.release = release_b_cas_card;
	r->super.init = init_b_cas_card;
	r->super.get_init_status = get_init_status_b_cas_card;
	r->super.proc_ecm = proc_ecm_b_cas_card;

	r->push = push;
	r->set_queue_len = set_queue_len;
	r->get_queue_current_len = get_queue_current_len;
	r->set_init_status = set_init_status;
	r->set_init_status_from_hex = set_init_status_from_hex;

	return r;
}

