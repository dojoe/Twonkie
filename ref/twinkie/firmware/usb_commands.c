/* Copyright 2017 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "common.h"
#include "config.h"
#include "console.h"
#include "link_defs.h"
#include "printf.h"
#include "registers.h"
#include "task.h"
#include "timer.h"
#include "util.h"
#include "usb_api.h"
#include "usb_descriptor.h"
#include "usb_hw.h"

/* Console output macro */
#define CPRINTF(format, args...) cprintf(CC_USB, format, ## args)

#define USB_COMMAND_TX_SIZE  512
#define USB_COMMAND_BUF_COUNT (DIV_ROUND_UP(USB_COMMAND_TX_SIZE, \
					    USB_MAX_PACKET_SIZE))
#define USB_COMMAND_TX_WORD_COUNT (USB_COMMAND_BUF_COUNT * \
				   USB_MAX_PACKET_SIZE / sizeof(usb_uint))

const struct usb_interface_descriptor USB_IFACE_DESC(USB_IFACE_COMMAND) = {
	.bLength            = USB_DT_INTERFACE_SIZE,
	.bDescriptorType    = USB_DT_INTERFACE,
	.bInterfaceNumber   = USB_IFACE_COMMAND,
	.bAlternateSetting  = 0,
	.bNumEndpoints      = 2,
	.bInterfaceClass    = USB_CLASS_VENDOR_SPEC,
	.bInterfaceSubClass = 0,
	.bInterfaceProtocol = 0,
	.iInterface         = USB_STR_COMMAND_NAME,
};
const struct usb_endpoint_descriptor USB_EP_DESC(USB_IFACE_COMMAND, 0) = {
	.bLength            = USB_DT_ENDPOINT_SIZE,
	.bDescriptorType    = USB_DT_ENDPOINT,
	.bEndpointAddress   = 0x80 | USB_EP_COMMAND,
	.bmAttributes       = 0x02 /* Bulk IN */,
	.wMaxPacketSize     = USB_MAX_PACKET_SIZE,
	.bInterval          = 10
};
const struct usb_endpoint_descriptor USB_EP_DESC(USB_IFACE_COMMAND, 1) = {
	.bLength            = USB_DT_ENDPOINT_SIZE,
	.bDescriptorType    = USB_DT_ENDPOINT,
	.bEndpointAddress   = USB_EP_COMMAND,
	.bmAttributes       = 0x02 /* Bulk OUT */,
	.wMaxPacketSize     = USB_MAX_PACKET_SIZE,
	.bInterval          = 0
};

static usb_uint ep_buf_tx[USB_COMMAND_TX_WORD_COUNT] __usb_ram;
static usb_uint ep_buf_rx[USB_MAX_PACKET_SIZE / sizeof(usb_uint)] __usb_ram;

static volatile unsigned tx_idx;
static volatile usb_uint tx_next_buf;
static volatile unsigned rx_count;
static int processing;

static void cmd_ep_tx(void)
{
	unsigned txlen;

	if (!tx_next_buf) { /* we are done, just clear IRQ */
		STM32_TOGGLE_EP(USB_EP_COMMAND, 0, 0, 0);
		return;
	}

	txlen = MIN(tx_idx, USB_MAX_PACKET_SIZE);
	btable_ep[USB_EP_COMMAND].tx_addr  = tx_next_buf;
	btable_ep[USB_EP_COMMAND].tx_count = txlen;
	tx_idx -= txlen;
	if (txlen == USB_MAX_PACKET_SIZE)
		tx_next_buf = btable_ep[USB_EP_COMMAND].tx_addr +
				USB_MAX_PACKET_SIZE;
	else
		tx_next_buf = 0;
	STM32_TOGGLE_EP(USB_EP_COMMAND, EP_TX_MASK, EP_TX_VALID, 0);
}

static void cmd_ep_rx(void)
{
	rx_count = btable_ep[USB_EP_COMMAND].rx_count & 0x3ff;

	/* clear IT without re-enabling RX */
	STM32_TOGGLE_EP(USB_EP_COMMAND, 0, 0, 0);

	/* wake-up the console task */
	console_has_input();
}

static void cmd_ep_reset(void)
{
	btable_ep[USB_EP_COMMAND].tx_addr  = usb_sram_addr(ep_buf_tx);
	btable_ep[USB_EP_COMMAND].tx_count = 0;

	btable_ep[USB_EP_COMMAND].rx_addr  = usb_sram_addr(ep_buf_rx);
	btable_ep[USB_EP_COMMAND].rx_count =
		0x8000 | ((USB_MAX_PACKET_SIZE / 32 - 1) << 10);

	STM32_USB_EP(USB_EP_COMMAND) = (USB_EP_COMMAND | /* Endpoint Addr */
					(2 << 4)       | /* TX NAK        */
					(0 << 9)       | /* Bulk EP       */
					EP_RX_VALID);
}

USB_DECLARE_EP(USB_EP_COMMAND, cmd_ep_tx, cmd_ep_rx, cmd_ep_reset);

/* we have space to insert the null terminator */
BUILD_ASSERT(CONFIG_CONSOLE_INPUT_LINE_SIZE > USB_MAX_PACKET_SIZE);

int console_packet_get_command(char *buf)
{
	unsigned count = rx_count;

	if (!rx_count)
		return 0;

	memcpy_from_usbram(buf, (void *) usb_sram_addr(ep_buf_rx), count);
	/* force a null termination */
	buf[count] = 0;
	rx_count = 0;
	tx_idx = 0;
	btable_ep[USB_EP_COMMAND].tx_count = 0;
	tx_next_buf = 0;
	processing = 1;

	return count;
}


void console_packet_send_response(int rv)
{
	unsigned txlen = MIN(tx_idx, USB_MAX_PACKET_SIZE);

	processing = 0;
	btable_ep[USB_EP_COMMAND].tx_addr  = usb_sram_addr(ep_buf_tx);
	btable_ep[USB_EP_COMMAND].tx_count = txlen;
	tx_idx -= txlen;
	if (txlen == USB_MAX_PACKET_SIZE)
		tx_next_buf = btable_ep[USB_EP_COMMAND].tx_addr +
				USB_MAX_PACKET_SIZE;
	else
		tx_next_buf = 0;
	/* ready to answer and re-enable reception */
	STM32_TOGGLE_EP(USB_EP_COMMAND, EP_TX_RX_MASK, EP_TX_RX_VALID, 0);
}

static int __tx_char(void *context, int c)
{
	if (tx_idx >= USB_COMMAND_TX_SIZE)
		return 1;
	if (!(tx_idx & 1))
		ep_buf_tx[tx_idx/2] = c;
	else
		ep_buf_tx[tx_idx/2] |= c << 8;
	tx_idx++;

	return 0;
}

static int in_command(enum console_channel channel)
{
	return (channel == CC_COMMAND) && processing;
}

int console_packet_puts(enum console_channel channel, const char *outstr)
{
	if (!in_command(channel))
		return EC_ERROR_UNIMPLEMENTED; /* Fallback */

	/* Put all characters in the output buffer */
	while (*outstr) {
		if (__tx_char(NULL, *outstr++) != 0)
			break;
	}

	/* Successful if we consumed all output */
	return *outstr ? EC_ERROR_OVERFLOW : EC_SUCCESS;
}

int console_packet_vprintf(enum console_channel channel, const char *format,
			   va_list args)
{
	if (!in_command(channel))
		return EC_ERROR_UNIMPLEMENTED; /* Fallback */

	return vfnprintf(__tx_char, NULL, format, args);
}
