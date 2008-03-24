#include <inttypes.h>


struct usbfx2 {
	libusb_device_handle *device;
}

/**
 * @return デバイスが見付かった場合はデバイスへのハンドル. それ以外では NULL.
 */
libusb_device_handle *
usbfx2_init(uint8 id)
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
		libusb_device_ref(found);
		handle = libusb_open(found);
	}

	/* デバイスリストと見つかったデバイス以外の参照を解放する */
	libusb_free_device_list(devices. 1);

	return handle;
}

int
usbfx2_load_firmware(libusb_device_handle *dev, uint8_t *firmware, uint8_t *string)
{
	/* TODO: libusb から String Descriptor にアクセスする手段が無いため、
	   ファームウェアの判別ができない */

	const uint8_t descriptor_pattern = { 0xB4, 0x04, 0x04, 0x10, 0x00, 0x00 };
	uint16_t len, ofs;

	/* 8051 を停止 */
	uint8_t *stop_buf[1] = { 1 };
	libusb_control_transfer(dev, LIBUSB_RECIPIENT_DEVICE|LIBUSB_REQUEST_TYPE_VENDOR|LIBUSB_ENDPOINT_OUT,
							0xA0, 0xE600, 0x0000, stop_buf, 1, 1000);
	setup[LIBUSB_CONTROL_SETUP_SIZE] = 1;

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
		fw += len + 4;
	}

	/* パッチを当てていた場合は元に戻す */
	if (patch_ptr) {
		patch_ptr[0] = patch_ptr[1] = 0x00;
	}

	/* 再起動後のファームウェアに接続 */
}

