#include <inttypes.h>


/* private */
struct cusbfx2_handle {
	libusb_device_handle *usb_handle;
};


/**
 * @param id
 * @return デバイスが見付かった場合はデバイスへのハンドル. それ以外では NULL.
 */
static libusb_device_handle *
usbfx2_find_and_open(uint8_t id)
{
	libusb_device *found = NULL;
	libusb_device **devices;
	libusb_device_handle *handle = NULL;
	int ndevices;
	int i = 0;

	ndevices = libusb_get_device_list(&list);
	if (ndevices < 0) {
		return NULL;
	}

	/* 接続されている全てのUSBデバイスを調べる */
	for (i = 0; i < ndevices; ++i) {
		libusb_device_descriptor *desc = libusb_get_device_descriptor(devices[i]);

		/* bcdDevice = FX2LP:0xA0nn FX2:0x00nn */
		if (((id == 0) && ((desc->bcdDevice & 0x0F00) == 0x0000)) ||
			(desc->bcdDevice == 0xFF00 + id)) {
			if ((desc->idVendor == 0x04B4) &&
				((desc->idProduct == 0x8613) || (desc->idProduct == 0x1004))) {
				found = devices[i];
				break;
			}
		}
	}

	if (found) {
		handle = libusb_open(found);
		libusb_claim_interface(handle, 0);
	}

	/* デバイスリストと見つかったデバイス以外の参照を解放する */
	libusb_free_device_list(devices. 1);

	return handle;
}


int
usbfx2_load_firmware(libusb_device_handle *dev, uint8_t id, uint8_t *firmware, const char *firmware_id)
{
	/* TODO: libusb から String Descriptor にアクセスする手段が無いため、
	   ファームウェアの判別ができない */

	static const uint8_t descriptor_pattern = { 0xB4, 0x04, 0x04, 0x10, 0x00, 0x00 };
	uint16_t len, ofs;

	/* 8051 を停止 */
	uint8_t *stop_buf[1] = { 1 };
	libusb_control_transfer(dev, LIBUSB_RECIPIENT_DEVICE|LIBUSB_REQUEST_TYPE_VENDOR|LIBUSB_ENDPOINT_OUT,
							0xA0, 0xE600, 0x0000, stop_buf, 1, 1000);

	firmware += 8;

	for (;;) {
		len = (firmware[0] << 8) | firmware[1];
		ofs = (firmware[2] << 8) | firmware[3];

		/* ファームウェアの Device Descript VID=04B4 PID=1004 BCD=0000 を見付け出し、
		   idに適合するBCD(FFxx)に書き換える */
		for (i = 0; i < (len & 0x7FFF); ++i) {
			for (j = 0; j < 6; ++j) {
				if (firmware[4 + i + j] != descriptor_pattern[j])
					break;
			}
			if (j >= 6) {
				/* パターンが見付かったので、BCDを書き換える */
				patch_ptr = firmware + 4 + i + 4;
				patch_ptr[0] = id;
				patch_ptr[1] = 0xFF;
			}
		}

		/* ベンダリクエスト'A0'を使用して8051に書き込む */
		libusb_control_transfer(dev, LIBUSB_RECIPIENT_DEVICE|LIBUSB_REQUEST_TYPE_VENDOR|LIBUSB_ENDPOINT_OUT,
								0xA0, ofs, 0x0000, firmware + 4, len & 0x7FFF, 1000);

		if (len & 0x8000)
			break;				/* 最終(リセット) */

		firmware += len + 4;
	}

	/* パッチを当てていた場合は元に戻す */
	if (patch_ptr) {
		patch_ptr[0] = patch_ptr[1] = 0x00;
	}
}

/* Exposed functions */

/**
 * Initialize cusbfx2.
 *
 * This function must be called before calling any other cusbfx2 functions.
 *
 * @return 0 on success, non-zero on error
 */
int
CUSBFX2_init(void)
{
	return libusb_init();
}


/**
 * Deinitialize cusbfx2.
 *
 * Should be called after closing all open devices and before your application terminates.
 */
void
CUSBFX2_exit(void)
{
	libusb_exit();
}


/**
 * ID が @a id である FX2 をオープンします。
 *
 * @a firmware が NULL であれば、ファームウェアのロードを行いません。
 *
 * @param id FX2のID
 * @param firmware ファームウェア
 * @param firmware_id ファームウェアのID
 * @return handle
 */
CUSBFX2_Handle *
CUSBFX2_open(uint8_t id, uint8_t *firmware, const char *firmware_id)
{
#define CUSBFX2_REOPEN_RETRY_MAX 400
#define CUSBFX2_REOPEN_RETRY_WAIT 100
	libusb_device_handle *dev;
	cusbfx2_handle *handle = NULL;

	dev = usbfx2_open(id);
	if (!dev) {
		return handle;
	}

	if (firmware) {
		usbfx2_load_firmware(dev, id, firmware, firmware_id);

		/* 再起動後のファームウェアに接続 */
		libusb_close(dev);
		libusb_device_unref(libusb_get_device(dev));

		for (i = 0; i < CUSBFX2_REOPEN_RETRY_MAX; ++i) {
			dev = usbfx2_open(id);
			if (dev)
				break;
			usleep(CUSBFX2_REOPEN_RETRY_WAIT);
		}
	}

	if (dev) {
		handle = malloc(sizeof(*handle));
		handle->usb_handle = dev;
	}

	return handle;
}

void
CUSBFX2_close(cusbfx2_handle *h)
{
	assert(handle);

	if (h->usb_handle) {
		if (libusb_release_interface(h->usb_handle)) {
			/* error */
		}

		if (libusb_close(h->usb_handle, 0)) {
			/* error */
		}
	}

	free(handle);
}
