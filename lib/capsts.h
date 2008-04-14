#ifndef CAPSTS_H_INCLUDED
#define CAPSTS_H_INCLUDED

enum {
	ENDPOINT_CMD_OUT	= 0x01,	/* Endpoint 1: Command from usb to firmware */
	ENDPOINT_TS_IN		= 0x86,	/* Endpoint 6: MPEG-TS from tuner to usb */
	ENDPOINT_BCAS_IN	= 0x84,	/* Endpoint 4: B-CAS from tuner to usb */
};

enum {
	/*
	 * ENDPOINT_TS_IN への出力を開始する
	 */
	CMD_EP6IN_START		= 0x50,
	/*
	 * ENDPOINT_TS_IN への出力を停止する
	 */
	CMD_EP6IN_STOP		= 0x51,
	/* unknown */
	CMD_EP2OUT_START	= 0x52,
	/* unknown */
	CMD_EP2OUT_STOP		= 0x53,
	/*
	 * @param addr_mask
	 * @param out_pins
	 */
	CMD_PORT_CFG		= 0x54,
	/*
	 * @param addr
	 * @return 1byte
	 */
	CMD_REG_READ		= 0x55,
	/*
	 * @param addr
	 * @param value
	 */
	CMD_REG_WRITE		= 0x56,
	/*
	 * @return 1byte
	 */
	CMD_PORT_READ		= 0x57,
	/*
	 * @param value
	 */
	CMD_PORT_WRITE		= 0x58,
	/*
	 * @param value
	 */
	CMD_IFCONFIG		= 0x59,
	/*  */
	CMD_MODE_IDLE		= 0x5a,
	/*
	 * ENDPOINT_BCAS_IN への出力を開始する
	 */
	CMD_EP4IN_START		= 0x5b,
	/*
	 * ENDPOINT_BCAS_IN への出力を停止する
	 */
	CMD_EP4IN_STOP		= 0x5c,
	/* 
	 * @param val_l
	 * @param val_h (0x0000:RBUF capture 0xffff:BWUF output)
	 */
	CMD_IR_CODE			= 0x5d,
	/* 
	 * @param ofs	0 or 64 or 128 or 192
	 * @param len	max 64
	 * @param data	len bytes
	 */
	CMD_IR_WBUF			= 0x5e,
	/* 
	 * @param ofs	0 or 64 or 128 or 192
	 * @return 64 bytes
	 */
	CMD_IR_RBUF			= 0x5f,
};

enum {
	PIO_START	= 0x20,
	PIO_IR_OUT	= 0x10,
	PIO_IR_IN	= 0x08,
	PIO_TS_BACK	= 0x04,
};

typedef enum CapStsIrCommand {
	IR_CMD_POWER_TOGGLE		= 0x048A,
	IR_CMD_POWER_ON			= 0x048B,
	IR_CMD_POWER_OFF		= 0x048C,
	IR_CMD_DISPLAY			= 0x048E,
	IR_CMD_MENU				= 0x048F,
	IR_CMD_1				= 0x0481,
	IR_CMD_2				= 0x0482,
	IR_CMD_3				= 0x0483,
	IR_CMD_4				= 0x0484,
	IR_CMD_5				= 0x0485,
	IR_CMD_6				= 0x0486,
	IR_CMD_7				= 0x0487,
	IR_CMD_8				= 0x0488,
	IR_CMD_9				= 0x0489,
	IR_CMD_0				= 0x0480,
	IR_CMD_11				= 0x04D1,
	IR_CMD_12				= 0x04D2,
	IR_CMD_SUBSCRIPTION		= 0x04D4,
	IR_CMD_FORMAT_TOGGLE	= 0x04D5,
	IR_CMD_DIGITAL_TERESTRIAL1	= 0x04D6,
	IR_CMD_BS_CS			= 0x04D7,
	IR_CMD_DIGITAL_TERESTRIAL2	= 0x04E1,
	IR_CMD_FORMAT_BS		= 0x04E2,
	IR_CMD_FORMAT_CS		= 0x04E3,
	IR_CMD_3DIGIT_INPUT		= 0x04D8,
	IR_CMD_CH_UP			= 0x04CD,
	IR_CMD_CH_DOWN			= 0x04CE,
	IR_CMD_EPG				= 0x04AE,
	IR_CMD_CURSOR_UP		= 0x0491,
	IR_CMD_CURSOR_DOWN		= 0x0492,
	IR_CMD_CURSOR_LEFT		= 0x0493,
	IR_CMD_CURSOR_RIGHT		= 0x0494,
	IR_CMD_ENTER			= 0x0495,
	IR_CMD_BACK				= 0x0496,
	IR_CMD_AUDIO_SELECT		= 0x049D,
	IR_CMD_ZOOM				= 0x049F,
	IR_CMD_VIDEO_SELECT		= 0x04C4,
	IR_CMD_DATA				= 0x04C7,
	IR_CMD_BLUE				= 0x04C8,
	IR_CMD_RED				= 0x04C9,
	IR_CMD_GREEN			= 0x04CA,
	IR_CMD_YELLOW			= 0x04CB,
	IR_CMD_PROGRAM_INFO		= 0x04CC,
} CapStsIrCommand;

/** チューナーの入力ソース  */
typedef enum CapStsTunerSource {
	TUNER_SOURCE_TERESTRIAL = 0, /* 地上波 */
	TUNER_SOURCE_BS,			/* BS */
	TUNER_SOURCE_CS,			/* CS */
	TUNER_SOURCE_MAX
} CapStsTunerSource;

/* Global functions
   ========================================================================== */

cusbfx2_handle *
capsts_open(gint fx2id, gboolean is_force_load);

void
capsts_cmd_push(guint8 cmd, ...);

gboolean
capsts_cmd_commit(cusbfx2_handle *device);

/* IR Interfaces
   -------------------------------------------------------------------------- */
void
capsts_set_ir_base(gint base);

void
capsts_ir_cmd_push(CapStsIrCommand cmd);

gboolean
capsts_ir_cmd_commit(cusbfx2_handle *device);

gboolean
capsts_adjust_tuner_channel(cusbfx2_handle *device, CapStsTunerSource source, const gchar *channel);

#endif	/* CAPSTS_H_INCLUDED */
