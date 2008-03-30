#include <stdlib.h>
#include <string.h>
#include <glib.h>

#include "bcas_stream.h"

struct BCASStream {
	GByteArray *raw_stream;		/* 未解析のバイトストリーム */
	gboolean is_synced;			/* ストリームの同期が取れているか? */
	guint n_sync_packets;		/*  */
};


#define PACKET_HEADER_SIZE 3
#define PACKET_MIN_SIZE (PACKET_HEADER_SIZE + 4 + 1 + 1 + 8 + 8 + 1)
#define PACKET_LEN_INDEX 2
#define PACKET_COMMAND_INDEX 3
#define PACKET_DATA_LEN_INDEX 7
#define PACKET_DATA_INDEX 8

#define PACKET_FLAG_INDEX 4
#define PACKET_KEY_INDEX 6
#define PACKET_KEY_SIZE (8 + 8)	/* KSo_odd + KSo_even */

#define IS_ECM_REQUEST(p)						\
	(((p)[PACKET_COMMAND_INDEX] == 0x90) &&		\
	 ((p)[PACKET_COMMAND_INDEX + 1] == 0x34) &&	\
	 ((p)[PACKET_COMMAND_INDEX + 2] == 0x00) &&	\
	 ((p)[PACKET_COMMAND_INDEX + 3] == 0x00))

static gboolean
bcas_stream_sync(BCASStream *self)
{
	guint8 *p;
	guint left_size, skip_size;
	gboolean is_synced = FALSE;

	/* バッファ長が絶対にパケットとして成立しない長さだと同期は取れない */
	if (self->raw_stream->len < PACKET_MIN_SIZE) {
		g_warning("bcas_stream_sync: not enough stream");
		return FALSE;
	}

	/* ECMコマンドが現われる場所を探す */
	left_size = self->raw_stream->len - PACKET_HEADER_SIZE + 4;
	skip_size = 0;
	for (p = self->raw_stream->data; left_size > 0; ++p, ++skip_size, --left_size) {
		if (IS_ECM_REQUEST(p)) {
			is_synced = TRUE;
			break;
		}
	}

	if (is_synced) {
		g_message("bcas_stream_sync: synced (skip %d bytes)", skip_size);

		/* ストリーム先頭の中途半端なパケットを削る */
		if (skip_size > 0) {
			self->raw_stream = g_byte_array_remove_range(self->raw_stream, 0, skip_size);
		}

		/* パケットサイズ分のデータが残っていなければまだ同期完了にしない */
		if (left_size < PACKET_HEADER_SIZE + p[PACKET_LEN_INDEX] + 1/* header + payload + checksum */) {
			g_warning("bcas_stream_sync: not enough stream (expect %d bytes but left %d bytes)",
					  p[PACKET_LEN_INDEX] + 1, left_size);
			is_synced = FALSE;
		}
	}

	return is_synced;
}

static void
bcas_stream_parse(BCASStream *self, BCASStreamCallbackFunc cbfn, gpointer user_data)
{
	BCASPacket packet;

	guint8 *p = self->raw_stream->data;
	guint left_size = self->raw_stream->len;
	guint parsed_size = 0;

	for (;;) {
		gint i;
		guint8 size, checksum, x = 0;

		/* 同期を取る */
		if (!self->is_synced) {
			self->is_synced = bcas_stream_sync(self);
			if (!self->is_synced) {
				g_warning("bcas_stream_parse: couldn't sync stream");
				break;
			}

			self->n_sync_packets = 0;
			p = self->raw_stream->data;
			left_size = self->raw_stream->len;
			parsed_size = 0;
		}

		/* 1パケット分のデータが残っていなければ終了 */
		if (left_size < PACKET_MIN_SIZE || left_size < p[PACKET_LEN_INDEX] + 1) {
			//g_warning("bcas_stream_parse: not enough stream");
			break;
		}

		size = PACKET_HEADER_SIZE + p[PACKET_LEN_INDEX] + 1/* checksum */;
		checksum = p[size - 1];

		/* パケットのチェックサムを計算 */
		for (i = 0; i < PACKET_HEADER_SIZE + p[PACKET_LEN_INDEX]; ++i)
			x ^= p[i];

		/* チェックサムが一致しなければ再同期 */
		if (x != checksum) {
			g_warning("bcas_stream_prase: broken packet");

			/* 解析済みパケットを削る(現パケットがECMコマンドであればそれも削る) */
			self->raw_stream = g_byte_array_remove_range(self->raw_stream, 0,
														 parsed_size + (IS_ECM_REQUEST(p) ? PACKET_COMMAND_INDEX + 4 : 0));

			self->is_synced = FALSE;
			continue;
		}

		/* ここまでくれば、パケットとしては正しいので、コールバック関数を呼ぶ */
		++self->n_sync_packets;

		packet.header = (p[0] << 8) | p[1];
		packet.len = p[2];
		memcpy(packet.payload, &p[3], packet.len);
		(*cbfn)(&packet, self->n_sync_packets == 1, user_data);

		p += size;
		parsed_size += size;
		left_size -= size;
	}
}

/* -------------------------------------------------------------------------- */
BCASStream *
bcas_stream_new(void)
{
	BCASStream *self;

	self = g_new(BCASStream, 1);
	self->raw_stream = g_byte_array_new();
	self->is_synced = FALSE;
	self->n_sync_packets = 0;

	return self;
}

void
bcas_stream_free(BCASStream *self)
{
	g_assert(self);
	
	g_byte_array_free(self->raw_stream, TRUE);
	g_free(self);
}

void
bcas_stream_push(BCASStream *self, guint8 *data, uint len, BCASStreamCallbackFunc cbfn, gpointer user_data)
{
	if (len > 0) {
		self->raw_stream = g_byte_array_append(self->raw_stream, data, len);
		bcas_stream_parse(self, cbfn, user_data);
	}
}



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
			guint32 commnad = 0x00150000; # 未確認
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
