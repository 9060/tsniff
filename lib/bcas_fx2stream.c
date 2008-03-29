#include "b_cas_card.h"
#include "b_cas_card_error_code.h"
#include "bcas_fx2stream.h"

#include <stdlib.h>
#include <string.h>
#include <glib.h>


static GAsyncQueue *st_async_queue;
static GByteArray *st_stream;

static GQueue *st_ecm_queue;

static B_CAS_INIT_STATUS st_bcas_init_status = {
	{ 0x36,0x31,0x04,0x66,0x4B,0x17,0xEA,0x5C,0x32,0xDF,0x9C,0xF5,0xC4,0xC3,0x6C,0x1B,
	  0xEC,0x99,0x39,0x21,0x68,0x9D,0x4B,0xB7,0xB7,0x4E,0x40,0x84,0x0D,0x2E,0x7D,0x98, },
	{ 0xFE,0x27,0x19,0x99,0x19,0x69,0x09,0x11, },
	0,
	0,
};
static gint st_ecm_queue_len = 128;


#define PACKET_HEADER_SIZE 3
#define PACKET_MIN_SIZE (PACKET_HEADER_SIZE + 4 + 1 + 1 + 8 + 8 + 1)
#define PACKET_LEN_INDEX 2
#define PACKET_COMMAND_INDEX 3
#define PACKET_DATA_LEN_INDEX 7
#define PACKET_DATA_INDEX 8

#define PACKET_FLAG_INDEX 4
#define PACKET_KEY_INDEX 6
#define PACKET_KEY_SIZE (8 + 8)	/* KSo_odd + KSo_even */

#define IS_ECM_REQUEST_PACKET(p) ((p)[3] == 0x90 && (p)[4] == 0x34 && (p)[5] == 0x00 && (p)[6] == 0x00)
#define IS_ECM_RESPONSE_PACKET(p) ((p)[0] == 0x00 && (p)[1] == 0x40 && (p)[2] == 0x19)

typedef struct ECMPacket {
	/* Requested packet */
	guint8 len;
	guint8 data[G_MAXUINT8];

	/* Responed packet */
	guint16 flag;
	guint8 key[PACKET_KEY_SIZE];
} ECMPacket;

static gboolean
bcas_stream_sync(GByteArray *stream)
{
	guint8 *p;
	guint left_size, skip_size;
	gboolean is_synced = FALSE;

	/* バッファ長が絶対にパケットとして成立しない長さだと同期は取れない */
	if (stream->len < PACKET_MIN_SIZE) {
		g_warning("bcas_stream_sync: not enough stream");
		return FALSE;
	}

	/* ECMコマンドが現われる場所を探す */
	left_size = stream->len - PACKET_HEADER_SIZE;
	skip_size = PACKET_HEADER_SIZE;
	for (p = stream->data + PACKET_HEADER_SIZE; left_size > 0; ++p, ++skip_size, --left_size) {
		if (IS_ECM_REQUEST_PACKET(p - PACKET_HEADER_SIZE)) {
			p -= PACKET_HEADER_SIZE; /* p はパケット先頭を指すように */
			is_synced = TRUE;
		}
	}

	if (is_synced) {
		g_message("bcas_stream_sync: synced (skip %d bytes)", skip_size - PACKET_HEADER_SIZE);

		/* ストリーム先頭の中途半端なパケットを削る */
		if (skip_size > PACKET_HEADER_SIZE) {
			g_byte_array_remove_range(stream, 0, skip_size - PACKET_HEADER_SIZE);
		}

		/* パケットサイズ分のデータが残っていなければまだ同期完了にしない */
		if (left_size < p[PACKET_LEN_INDEX] + 1/* header + payload + checksum */) {
			g_warning("bcas_stream_sync: not enough stream (expect %d bytes but left %d bytes)",
					  p[PACKET_LEN_INDEX] + 1, left_size);
			is_synced = FALSE;
		}
	}

	return is_synced;
}

static void
bcas_stream_push(void *data, uint len)
{
	static gboolean is_synced = FALSE;
	static ECMPacket *ecm_packet = NULL;
	static gint response_delay;

	guint8 *p = st_stream->data;
	guint left_size = st_stream->len;
	guint parsed_size = 0;

	g_byte_array_append(st_stream, data, len);

	for (;;) {
		gint i;
		guint8 size, checksum, x = 0;

		/* 同期を取る */
		if (!is_synced) {
			is_synced = bcas_stream_sync(st_stream);
			if (!is_synced) {
				g_warning("bcas_stream_push: couldn't sync stream");
				break;
			}
			p = st_stream->data;
			left_size = st_stream->len;
			parsed_size = 0;
		}

		/* 1パケット分のデータが残っていなければ終了 */
		if (left_size < PACKET_MIN_SIZE || left_size < p[PACKET_LEN_INDEX] + 1) {
			g_warning("bcas_stream_push: not enough stream");
			break;
		}

		size = p[PACKET_LEN_INDEX] + 1/* checksum */;
		checksum = p[size - 1];

		/* パケットのチェックサムを計算 */
		for (i = 0; i < p[PACKET_LEN_INDEX] + PACKET_HEADER_SIZE; ++i)
			x ^= p[i];

		/* チェックサムが一致しなければ再同期 */
		if (x != checksum) {
			g_warning("bcas_stream_push: broken packet");

			/* 解析済みパケットを削る(現パケットがECMコマンドであればそれも削る) */
			g_byte_array_remove_range(st_stream, 0,
									  parsed_size + (IS_ECM_COMMAND(p) ? PACKET_COMMAND_INDEX + 4 : 0));

			/* ECM Response パケット待ちであれば破棄する */
			if (ecm_packet) {
				g_warning("bcas_stream_push: dispose pending ECM request packet");
				g_slice_free(ECMPacket, ecm_packet);
				ecm_packet = NULL;
			}
			is_synced = FALSE;
			continue;
		}

		/* ここまでくれば、パケットとしては正しい */
		++response_delay;

		if (IS_ECM_REQUEST_PACKET(p)) {
			/* ECM Request パケットであれば対応する ECM Response を待つ */
			/* 既に Response を待っている? */
			if (ecm_packet) {
				g_warning("bcas_stream_push: found a new ECM request before completing old request (delta %d packets)",
						  response_delay - 1);
			} else {
				ecm_packet = g_slice_new(ECMPacket);
			}
			ecm_packet->len = p[PACKET_DATA_LEN_INDEX];
			g_memcpy(ecm_packet->data, &p[PACKET_DATA_INDEX], ecm_packet->len);

			response_delay = 0;
		} else if (IS_ECM_RESPONSE_PACKET(p)) {
			/* Request がないのに Response が来た */
			if (!ecm_packet) {
				g_warning("bcas_stream_push: not requested ECM response found");
			} else {
				if (response_delay > 1) {
					g_warning("bcas_stream_push: ECM response delayed by %d packets, maybe incorrect",
							  response_delay - 1);
				}
				ecm_packet->flag = (p[PACKET_FLAG_INDEX] << 8) | p[PACKET_FLAG_INDEX + 1];
				g_memcpy(ecm_packet->key, &p[PACKET_KEY_INDEX], PACKET_KEY_SIZE);

				/* ECMキューに追加 */
				g_queue_push_tail(st_ecm_queue, ecm_packet);
				if (g_gueue_get_length(st_ecm_queue) > st_ecm_queue_len) {
					/* ECMキューの最大長を越えていたら、最も古い ECM を捨てる */
					g_slice_free(ECMPacket, g_queue_pop_head(st_ecm_queue));
				}

				ecm_packet = NULL;
			}
		}

		p += size;
		parsed_size += size;
		left_size -= size;
	}
}

/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 interface method implementation
 ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/
static int init_b_cas_card(void *bcas)
{
	st_ecm_queue = g_queue_new();

	return 0;
}

static void release_b_cas_card(void *bcas)
{
	gpointer ecm;
	while (ecm = g_queue_pop_head(st_ecm_queue)) {
		g_free(ecm);
	}
	g_free(st_ecm_queue);

	g_free(bcas);
}

static int get_init_status_b_cas_card(void *bcas, B_CAS_INIT_STATUS *stat)
{
	memcpy(stat, &(st_bcas_init_status), sizeof(B_CAS_INIT_STATUS));
	return 0;
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

static int proc_ecm_b_cas_card(void *bcas, B_CAS_ECM_RESULT *dst, uint8_t *src, int len)
{
	gpointer data;
	ECMPacket src_packet;
	ECMPacket *ecm;

	/* ECMキューからECMパケットを検索 */
	src_packet.len = len;
	g_memcpy(src_packet.data, src, len);
	ecm = g_queue_find_custom(st_ecm_queue, &src_packet, compare_ecm_packet);

	if (ecm) {
		g_memcpy(dst->scramble_key, ecm->key, PACKET_KEY_SIZE);
		dst->return_code = 0x0200;
	} else {
	}

	return 0;
}


/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 global function implementation
 ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/
B_CAS_CARD *create_b_cas_card()
{
	B_CAS_CARD *r;

	r = g_new(B_CAS_CARD, 1);

	r->release = release_b_cas_card;
	r->init = init_b_cas_card;
	r->get_init_status = get_init_status_b_cas_card;
	r->proc_ecm = proc_ecm_b_cas_card;

	return r;
}

