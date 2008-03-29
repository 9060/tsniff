/*
 * Core functions for libusb
 * Copyright (C) 2007-2008 Daniel Drake <dsd@gentoo.org>
 * Copyright (c) 2001 Johannes Erdfelt <johannes@erdfelt.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

#include <config.h>

#include <errno.h>
#include <poll.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "libusb.h"
#include "libusbi.h"

#ifdef OS_LINUX
const struct usbi_os_backend * const usbi_backend = &linux_usbfs_backend;
#else
#error "Unsupported OS"
#endif

static struct list_head usb_devs;
struct list_head usbi_open_devs;

/**
 * \mainpage libusb-1.0 API Reference
 * libusb is an open source library that allows you to communicate with USB
 * devices from userspace. For more info, see the
 * <a href="http://libusb.sourceforge.net">libusb homepage</a>.
 *
 * This documentation is aimed at application developers wishing to
 * communicate with USB peripherals from their own software. After reviewing
 * this documentation, feedback and questions can be sent to the
 * <a href="http://sourceforge.net/mail/?group_id=1674">libusb-devel mailing
 * list</a>.
 */

/**
 * @defgroup lib Library initialization/deinitialization
 * This page details how to initialize and deinitialize libusb. Initialization
 * must be performed before using any libusb functionality, and similarly you
 * must not call any libusb functions after deinitialization.
 */

/**
 * @defgroup dev Device handling and enumeration
 * The functionality documented below is designed to help with the following
 * operations:
 * - Enumerating the USB devices currently attached to the system
 * - Choosing a device to operate from your software
 * - Opening and closing the chosen device
 *
 * \section nutshell In a nutshell...
 *
 * The description below really makes things sound more complicated than they
 * actually are. The following sequence of function calls will be suitable
 * for almost all scenarios and does not require you to have such a deep
 * understanding of the resource management issues:
 * \code
// discover devices
libusb_device **list;
libusb_device *found;
int cnt = libusb_get_device_list(&list);
int i = 0;
if (cnt < 0)
	error();

for (i = 0; i < cnt; i++) {
	libusb_device *device = list[i];
	if (is_interesting(device)) {
		found = device;
		break;
	}
}

if (found) {
	libusb_device_handle *handle = libusb_open(found);
	// etc
}

libusb_free_device_list(list, 1);
\endcode
 *
 * The two important points:
 * - You asked libusb_free_device_list() to unreference the devices (2nd
 *   parameter)
 * - You opened the device before freeing the list and unreferencing the
 *   devices
 *
 * If you ended up with a handle, you can now proceed to perform I/O on the
 * device.
 *
 * \section devshandles Devices and device handles
 * libusb has a concept of a USB device, represented by the
 * <tt>libusb_device</tt> opaque type. A device represents a USB device that
 * is currently or was previously connected to the system. Using a reference
 * to a device, you can determine certain information about the device (e.g.
 * you can read the descriptor data).
 *
 * The libusb_get_device_list() function can be used to obtain a list of
 * devices currently connected to the system. This is known as device
 * discovery.
 *
 * Just because you have a reference to a device does not mean it is
 * necessarily usable. The device may have been unplugged, you may not have
 * permission to operate such device, or another program or driver may be
 * using the device.
 *
 * When you've found a device that you'd like to operate, you must ask
 * libusb to open the device using the libusb_open() function. Assuming
 * success, libusb then returns you a <em>device handle</em>
 * (a <tt>libusb_device_handle</tt> pointer). All "real" I/O operations then
 * operate on the handle rather than the original device pointer.
 *
 * \section devref Device discovery and reference counting
 *
 * Device discovery (i.e. calling libusb_get_device_list()) returns a
 * freshly-allocated list of devices. The list itself must be freed when
 * you are done with it. libusb also needs to know when it is OK to free
 * the contents of the list - the devices themselves.
 *
 * To handle these issues, libusb provides you with two separate items:
 * - A function to free the list itself
 * - A reference counting system for the devices inside
 *
 * New devices presented by the libusb_get_device_list() function all have a
 * reference count of 1. You can increase and decrease reference count using
 * libusb_device_ref() and libusb_device_unref(). A device is destroyed when
 * it's reference count reaches 0.
 *
 * With the above information in mind, the process of opening a device can
 * be viewed as follows:
 * -# Discover devices using libusb_get_device_list().
 * -# Choose the device that you want to operate, and call libusb_open().
 * -# Unref all devices in the discovered device list.
 * -# Free the discovered device list.
 *
 * The order is important - you must not unreference the device before
 * attempting to open it, because unreferencing it may destroy the device.
 *
 * For convenience, the libusb_free_device_list() function includes a
 * parameter to optionally unreference all the devices in the list before
 * freeing the list itself. This combines steps 3 and 4 above.
 *
 * As an implementation detail, libusb_open() actually adds a reference to
 * the device in question. This is because the device is available later
 * through libusb_get_device(). The reference is deleted during libusb_close().
 */

/**
 * @defgroup misc Miscellaneous structures and constants
 * This page documents structures and constants that don't belong anywhere
 * else
 */

/* we traverse usbfs without knowing how many devices we are going to find.
 * so we create this discovered_devs model which is similar to a linked-list
 * which grows when required. it can be freed once discovery has completed,
 * eliminating the need for a list node in the libusb_device structure
 * itself. */
#define DISCOVERED_DEVICES_SIZE_STEP 8

static struct discovered_devs *discovered_devs_alloc(void)
{
	struct discovered_devs *ret =
		malloc(sizeof(*ret) + (sizeof(void *) * DISCOVERED_DEVICES_SIZE_STEP));

	if (ret) {
		ret->len = 0;
		ret->capacity = DISCOVERED_DEVICES_SIZE_STEP;
	}
	return ret;
}

/* append a device to the discovered devices collection. may realloc itself,
 * returning new discdevs. returns NULL on realloc failure. */
struct discovered_devs *discovered_devs_append(
	struct discovered_devs *discdevs, struct libusb_device *dev)
{
	size_t len = discdevs->len;
	size_t capacity;

	/* if there is space, just append the device */
	if (len < discdevs->capacity) {
		discdevs->devices[len] = libusb_device_ref(dev);
		discdevs->len++;
		return discdevs;
	}

	/* exceeded capacity, need to grow */
	usbi_dbg("need to increase capacity");
	capacity = discdevs->capacity + DISCOVERED_DEVICES_SIZE_STEP;
	discdevs = realloc(discdevs,
		sizeof(*discdevs) + (sizeof(void *) * capacity));
	if (discdevs) {
		discdevs->capacity = capacity;
		discdevs->devices[len] = libusb_device_ref(dev);
		discdevs->len++;
	}

	return discdevs;
}

static void discovered_devs_free(struct discovered_devs *discdevs)
{
	size_t i;

	for (i = 0; i < discdevs->len; i++)
		libusb_device_unref(discdevs->devices[i]);

	free(discdevs);
}

struct libusb_device *usbi_alloc_device(unsigned long session_id)
{
	size_t priv_size = usbi_backend->device_priv_size;
	struct libusb_device *dev = malloc(sizeof(*dev) + priv_size);
	if (!dev)
		return NULL;

	dev->refcnt = 1;
	dev->session_data = session_id;
	list_add(&dev->list, &usb_devs);
	memset(&dev->os_priv, 0, priv_size);
	return dev;
}

struct libusb_device *usbi_get_device_by_session_id(unsigned long session_id)
{
	struct libusb_device *dev;

	list_for_each_entry(dev, &usb_devs, list)
		if (dev->session_data == session_id)
			return dev;

	return NULL;
}

/** @ingroup dev
 * Returns a list of USB devices currently attached to the system. This is
 * your entry point into finding a USB device to operate.
 *
 * You are expected to unreference all the devices when you are done with
 * them, and then free the list with libusb_free_device_list(). Note that
 * libusb_free_device_list() can unref all the devices for you. Be careful
 * not to unreference a device you are about to open until after you have
 * opened it.
 *
 * \param list output location for a list of devices. Must be later freed with
 * libusb_free_device_list().
 * \returns the number of devices in the outputted list, or negative on error
 */
API_EXPORTED int libusb_get_device_list(struct libusb_device ***list)
{
	struct discovered_devs *discdevs = discovered_devs_alloc();
	struct libusb_device **ret;
	int r = 0;
	size_t i;
	size_t len;
	usbi_dbg("");

	if (!discdevs)
		return -ENOMEM;

	r = usbi_backend->get_device_list(&discdevs);
	if (r < 0)
		goto out;

	/* convert discovered_devs into a list */
	len = discdevs->len;
	ret = malloc(sizeof(void *) * (len + 1));
	if (!ret) {
		r = -ENOMEM;
		goto out;
	}

	ret[len] = NULL;
	for (i = 0; i < len; i++) {
		struct libusb_device *dev = discdevs->devices[i];
		ret[i] = libusb_device_ref(dev);
	}
	*list = ret;

out:
	discovered_devs_free(discdevs);
	return r;
}

/** \ingroup dev
 * Frees a list of devices previously discovered using
 * libusb_get_device_list(). If the unref_devices parameter is set, the
 * reference count of each device in the list is decremented by 1.
 * \param list the list to free
 * \param unref_devices whether to unref the devices in the list
 */
API_EXPORTED void libusb_free_device_list(struct libusb_device **list,
	int unref_devices)
{
	if (!list)
		return;

	if (unref_devices) {
		int i = 0;
		struct libusb_device *dev;

		while ((dev = list[i++]) != NULL)
			libusb_device_unref(dev);
	}
	free(list);
}

/** \ingroup dev
 * Increment the reference count of a device.
 * \param dev the device to reference
 * \returns the same device
 */
API_EXPORTED struct libusb_device *libusb_device_ref(struct libusb_device *dev)
{
	dev->refcnt++;
	return dev;
}

/** \ingroup dev
 * Decrement the reference count of a device. If the decrement operation
 * causes the reference count to reach zero, the device shall be destroyed.
 * \param dev the device to unreference
 */
API_EXPORTED void libusb_device_unref(struct libusb_device *dev)
{
	if (!dev)
		return;

	if (--dev->refcnt == 0) {
		usbi_dbg("destroy device %04x:%04x", dev->desc.idVendor,
			dev->desc.idProduct);

		if (usbi_backend->destroy_device)
			usbi_backend->destroy_device(dev);

		list_del(&dev->list);
		if (dev->config)
			free(dev->config);
		free(dev);
	}
}

/** \ingroup dev
 * Open a device and obtain a device handle. A handle allows you to perform
 * I/O on the device in question.
 *
 * Internally, this function adds a reference to the device and makes it
 * available to you through libusb_get_device(). This reference is removed
 * during libusb_close().
 *
 * \param dev the device to open
 * \returns a handle for the device, or NULL on error
 */
API_EXPORTED struct libusb_device_handle *libusb_open(struct libusb_device *dev)
{
	struct libusb_device_handle *handle;
	size_t priv_size = usbi_backend->device_handle_priv_size;
	int r;
	usbi_dbg("open %04x:%04x", dev->desc.idVendor, dev->desc.idProduct);

	handle = malloc(sizeof(*handle) + priv_size);
	if (!handle)
		return NULL;

	handle->dev = libusb_device_ref(dev);
	memset(&handle->os_priv, 0, priv_size);
	r = usbi_backend->open(handle);
	if (r < 0) {
		libusb_device_unref(dev);
		free(handle);
		return NULL;
	}

	list_add(&handle->list, &usbi_open_devs);
	return handle;
}

/** \ingroup dev
 * Convenience function for finding a device with a particular
 * <tt>idVendor</tt>/<tt>idProduct</tt> combination. This function is intended
 * for those scenarios where you are using libusb to knock up a quick test
 * application - it allows you to avoid calling libusb_get_device_list() and
 * worrying about traversing/freeing the list.
 *
 * This function has limitations and is hence not intended for use in real
 * applications: if multiple devices have the same IDs it will only
 * give you the first one, etc.
 *
 * \param vendor_id the idVendor value to search for
 * \param product_id the idProduct value to search for
 * \returns a handle for the first found device, or NULL on error or if the
 * device could not be found. */
API_EXPORTED struct libusb_device_handle *libusb_open_device_with_vid_pid(
	uint16_t vendor_id, uint16_t product_id)
{
	struct libusb_device **devs;
	struct libusb_device *found = NULL;
	struct libusb_device *dev;
	struct libusb_device_handle *handle = NULL;
	size_t i = 0;

	if (libusb_get_device_list(&devs) < 0)
		return NULL;

	while ((dev = devs[i++]) != NULL) {
		struct libusb_device_descriptor *desc =
			libusb_get_device_descriptor(dev);
		if (desc->idVendor == vendor_id && desc->idProduct == product_id) {
			found = dev;
			break;
		}
	}

	if (found)
		handle = libusb_open(found);

	libusb_free_device_list(devs, 1);
	return handle;
}

static void do_close(struct libusb_device_handle *dev_handle)
{
	usbi_backend->close(dev_handle);
	libusb_device_unref(dev_handle->dev);
}

/** \ingroup dev
 * Close a device handle. Should be called on all open handles before your
 * application exits.
 *
 * Internally, this function destroys the reference that was added by
 * libusb_open() on the given device.
 *
 * \param dev_handle the handle to close
 */
API_EXPORTED void libusb_close(struct libusb_device_handle *dev_handle)
{
	if (!dev_handle)
		return;
	usbi_dbg("");

	list_del(&dev_handle->list);
	do_close(dev_handle);
	free(dev_handle);
}

/** \ingroup dev
 * Get the underlying device for a handle. This function does not modify
 * the reference count of the returned device, so do not feel compelled to
 * unreference it when you are done.
 * \param dev_handle a device handle
 * \returns the underlying device
 */
API_EXPORTED struct libusb_device *libusb_get_device(
	struct libusb_device_handle *dev_handle)
{
	return dev_handle->dev;
}

/* FIXME: what about claiming multiple interfaces? */
/** \ingroup dev
 * Claim an interface on a given device handle. You must claim the interface
 * you wish to use before you can perform I/O on any of the endpoints.
 * \param iface the <tt>bInterfaceNumber</tt> of the interface you wish to claim
 * \param dev a device handle
 * \returns 0 on success, non-zero on error
 */
API_EXPORTED int libusb_claim_interface(struct libusb_device_handle *dev,
	int iface)
{
	usbi_dbg("interface %d", iface);
	return usbi_backend->claim_interface(dev, iface);
}

/** \ingroup dev
 * Release an interface previously claimed with libusb_claim_interface(). You
 * should release all claimed interfaces before closing a device handle.
 * \param dev a device handle
 * \param iface the <tt>bInterfaceNumber</tt> of the previously-claimed
 * interface
 * \returns 0 on success, non-zero on error
 */
API_EXPORTED int libusb_release_interface(struct libusb_device_handle *dev,
	int iface)
{
	usbi_dbg("interface %d", iface);
	return usbi_backend->release_interface(dev, iface);
}

/** \ingroup lib
 * Initialize libusb. This function must be called before calling any other
 * libusb function.
 * \returns 0 on success, non-zero on error
 */
API_EXPORTED int libusb_init(void)
{
	usbi_dbg("");

	if (usbi_backend->init) {
		int r = usbi_backend->init();
		if (r < 0)
			return r;
	}

	list_init(&usb_devs);
	list_init(&usbi_open_devs);
	usbi_io_init();
	return 0;
}

/** \ingroup lib
 * Deinitialize libusb. Should be called after closing all open devices and
 * before your application terminates.
 */
API_EXPORTED void libusb_exit(void)
{
	struct libusb_device_handle *devh;
	usbi_dbg("");
	if (!list_empty(&usbi_open_devs)) {
		usbi_dbg("naughty app left some devices open!\n");
		list_for_each_entry(devh, &usbi_open_devs, list)
			do_close(devh);
		/* FIXME where do the open handles get freed? */
	}
	if (usbi_backend->exit)
		usbi_backend->exit();
}

void usbi_log(enum usbi_log_level level, const char *function,
	const char *format, ...)
{
	va_list args;
	FILE *stream = stdout;
	const char *prefix;

	switch (level) {
	case LOG_LEVEL_INFO:
		prefix = "info";
		break;
	case LOG_LEVEL_WARNING:
		stream = stderr;
		prefix = "warning";
		break;
	case LOG_LEVEL_ERROR:
		stream = stderr;
		prefix = "error";
		break;
	case LOG_LEVEL_DEBUG:
		stream = stderr;
		prefix = "debug";
		break;
	default:
		stream = stderr;
		prefix = "unknown";
		break;
	}

	fprintf(stream, "libusb:%s [%s] ", prefix, function);

	va_start (args, format);
	vfprintf(stream, format, args);
	va_end (args);

	fprintf(stream, "\n");
}

