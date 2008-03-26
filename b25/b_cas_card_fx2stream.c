#include "b_cas_card.h"
#include "b_cas_card_error_code.h"
#include "b_cas_card_fx2stream.h"

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

/* 
   B-CAS Stream Format

   We are intersting only ECM command.

	request_packet {  # big endian and packed
		header {
			guint8 head1 = 0x00;
			guint8 head2 = 0x40;
			guint8 len = sizeof(payload);
		}
		payload {
			guint32 command = 0x90340000;
			guint8 data_len;
			guint8 data[data_len];
		}
		guint8 checksum;	# XOR-ed packet bytes
	};

	response_packet {	# always lead by request_packet
		header {
			guint8 head1 = 0x00;
			guint8 head2 = 0x40;
			guint8 len = sizeof(payload);
		}
		payload {
			guint32 commnad = 0x00150000; # ̤��ǧ
			guint16 flag; # 0x0200 = Payment-deferred PPV / 0x0400 = Prepaid PPV / 0x0800 = Tier
			guint8 KSo_odd[8];
			guint8 KSo_even[8];
			guint8 unknown1;
			guint8 unknown2;
			guint8 unknown3;
		}
	};


00000000  00 40 24
          90 34 00 00 command
          1e data_len
          00 1e 01 fe cc a0 87 95 -
00000010  be 5b 1a f4 ea b3 70 b4  d4 e4 81 b4 2e 62 c3 31 -
00000020  38 99 42 e0 b3 85 00
          59

          00 40 19
          00 15 00 00
          08 -
00000030  00
          af e2 83 4f 9c 79 c0  d9 10 4c b4 36 e6 70 a3
00000040  8f
          01 90 00
          cc

          00 00 07
          90 80 00 00 01 00 00
          16

00000050  00 00 14
          00 10 00 00 a1  01 00 00 00 00 00 00 00
00000060  00 00 00 00 00 90 00
          34

          00 40 24
          90 34 00 00 1e
00000070  00 1e 01 e3 bd 95 79 38  65 8b 90 19 11 c4 87 6e
 */

/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 private method implementation
 ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/

#define PACKET_HEADER_SIZE 3
#define PACKET_MIN_SIZE (PACKET_HEADER_SIZE + 4 + 1 + 1 + 8 + 8 + 1)
#define PACKET_LEN_INDEX 2
#define PACKET_COMMAND_INDEX 3
#define PACKET_DATA_LEN_INDEX 7
#define PACKET_DATA_INDEX 8

#define PACKET_FLAG_INDEX 4
#define PACKET_KEY_INDEX 6
#define PACKET_KEY_SIZE (8 + 8)	/* KSo_odd + KSo_even */

#define IS_ECM_REQUEST_PACKET(p) (p[3] == 0x90 && p[4] == 0x34 && p[5] == 0x00 && p[6] == 0x00)
#define IS_ECM_RESPONSE_PACKET(p) (p[0] == 0x00 && p[1] == 0x40 && p[2] == 0x19)

typedef struct {
	/* Requested packet */
	uint8 len;
	uint8 data[G_UINT8_MAX];

	/* Responed packet */
	uint16 flag;
	uint8 key[PACKET_KEY_SIZE];
} ECMPacket;

static gboolean
bcas_stream_sync(GByteArray *stream)
{
	guint8 *p;
	guint left_size, skip_size;
	gboolean is_synced = FALSE;

	/* �Хåե�Ĺ�����Ф˥ѥ��åȤȤ�����Ω���ʤ�Ĺ������Ʊ���ϼ��ʤ� */
	if (stream->len < PACKET_MIN_SIZE) {
		g_warning("bcas_stream_sync: not enough stream");
		return FALSE;
	}

	/* ECM���ޥ�ɤ����������õ�� */
	left_size = stream->len - PACKET_HEADER_SIZE;
	skip_size = PACKET_HEADER_SIZE;
	for (p = stream->data + PACKET_HEADER_SIZE; left_size > 0; ++p, ++skip_size, --left_size) {
		if (IS_ECM_REQUEST_PACKET(p - PACKET_HEADER_SIZE)) {
			p -= PACKET_HEADER_SIZE; /* p �ϥѥ��å���Ƭ��ؤ��褦�� */
			is_synced = TRUE;
		}
	}

	if (is_synced) {
		g_message("bcas_stream_sync: synced (skip %d bytes)", skip_size - PACKET_HEADER_SIZE);

		/* ���ȥ꡼����Ƭ������Ⱦü�ʥѥ��åȤ��� */
		if (skip_size > PACKET_HEADER_SIZE) {
			g_byte_array_remove_range(stream, 0, skip_size - PACKET_HEADER_SIZE);
		}

		/* �ѥ��åȥ�����ʬ�Υǡ������ĤäƤ��ʤ���Фޤ�Ʊ����λ�ˤ��ʤ� */
		if (left_size < p[PACKET_LEN_INDEX] + 1/* header + payload + checksum */) {
			g_warning("bcas_stream_sync: not enough stream (expect %d bytes but left %d bytes)",
					  p[PACKET_LEN_INDEX] + 1, left_size);
			is_synced = FALSE;
		}
	}

	return is_synced;
}

static void
bcas_stream_push(B_CAS_CARD_CHUNK *chunk)
{
	static gboolean is_synced = FALSE;
	static ECMPacket *ecm_packet = NULL;
	static gint response_delay;

	guint8 *p = st_stream->data;
	guint left_size = st_stream->len;
	guint parsed_size = 0;

	g_byte_array_append(st_stream, chunk->data, chunk->len);

	for (;;) {
		gint i;
		guint8 size, checksum, x = 0;

		/* Ʊ������ */
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

		/* 1�ѥ��å�ʬ�Υǡ������ĤäƤ��ʤ���н�λ */
		if (left_size < PACKET_MIN_SIZE || left_size < p[PACKET_LEN_INDEX] + 1) {
			g_warning("bcas_stream_push: not enough stream");
			break;
		}

		size = p[PACKET_LEN_INDEX] + 1/* checksum */;
		checksum = p[size - 1];

		/* �ѥ��åȤΥ����å������׻� */
		for (i = 0; i < p[PACKET_LEN_INDEX] + HEADER_SIZE; ++i)
			x ^= p[i];

		/* �����å����ब���פ��ʤ���к�Ʊ�� */
		if (x != checksum) {
			g_warning("bcas_stream_push: broken packet");

			/* ���ϺѤߥѥ��åȤ���(���ѥ��åȤ�ECM���ޥ�ɤǤ���Ф������) */
			g_byte_array_remove_range(st_stream, 0,
									  parsed_size + (IS_ECM_COMMAND(command) ? PACKET_COMMAND_INDEX + 4 : 0));

			/* ECM Response �ѥ��å��Ԥ��Ǥ�����˴����� */
			if (ecm_packet) {
				g_warning("bcas_stream_push: dispose pending ECM request packet");
				g_slice_free(ecm_packet, ECMPacket);
				ecm_packet = NULL;
			}
			is_synced = FALSE;
			continue;
		}

		/* �����ޤǤ���С��ѥ��åȤȤ��Ƥ������� */
		++response_delay;

		if (IS_ECM_REQUEST_PACKET(p)) {
			/* ECM Request �ѥ��åȤǤ�����б����� ECM Response ���Ԥ� */
			/* ���� Response ���ԤäƤ���? */
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
			/* Request ���ʤ��Τ� Response ���褿 */
			if (!ecm_packet) {
				g_warning("bcas_stream_push: not requested ECM response found");
			} else {
				if (response_delay > 1) {
					g_warning("bcas_stream_push: ECM response delayed by %d packets, maybe incorrect",
							  response_delay - 1);
				}
				ecm_packet->flag = (p[PACKET_FLAG_INDEX] << 8) | p[PACKET_FLAG_INDEX + 1];
				g_memcpy(ecm_packet->key, &p[PACKET_KEY_INDEX], PACKET_KEY_SIZE);

				/* ECM���塼���ɲ� */
				g_queue_push_tail(st_ecm_queue, ecm_packet);
				if (g_gueue_get_length(st_ecm_queue) > st_ecm_queue_len) {
					/* ECM���塼�κ���Ĺ��ۤ��Ƥ����顢�Ǥ�Ť� ECM ��ΤƤ� */
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
	int m;
	LONG ret;
	DWORD len;
	
	B_CAS_CARD_PRIVATE_DATA *prv;

	prv = private_data(bcas);
	if(prv == NULL){
		return B_CAS_CARD_ERROR_INVALID_PARAMETER;
	}

	st_ecm_queue = g_queue_new();

	return 0;
}

static void release_b_cas_card(void *bcas)
{
	gpointer ecm;
	while (ecm = g_queue_pop_head()) {
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

static int proc_ecm_b_cas_card(void *bcas, B_CAS_ECM_RESULT *dst, uint8_t *src, int len)
{
	gpinter data;
	data = g_async_queue_try_pop(st_bcas_queue);
	if (data) {
		
	}

	memcpy(dst->scramble_key, prv->rbuf+6, 16);
	dst->return_code = 0x0200;

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

