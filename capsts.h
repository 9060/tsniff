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


void
capsts_exec_cmd(guint8 cmd, ...);

void
capsts_exec_cmd_queue(cusbfx2_handle *device);

#endif	/* CAPSTS_H_INCLUDED */
