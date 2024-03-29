/* Copyright (c) 2014 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/* Twinkie dongle configuration */

#ifndef __CROS_EC_BOARD_H
#define __CROS_EC_BOARD_H

/* 48 MHz SYSCLK clock frequency */
#define CPU_CLOCK 48000000

/* the UART console is on USART1 (PA9/PA10) */
#define CONFIG_UART_CONSOLE 1

/* Optional features */
#define CONFIG_CONSOLE_PACKETS
#define CONFIG_USB
#define CONFIG_USB_BOS
#define CONFIG_USB_CONSOLE
#define CONFIG_USB_SERIALNO
/* Replaced at runtime (board_read_serial) by chip unique-id-based number. */
#define DEFAULT_SERIALNO ""
#define CONFIG_WEBUSB_URL "twebkie.org"

#define CONFIG_USB_POWER_DELIVERY
#define CONFIG_USB_PD_ALT_MODE
#define CONFIG_USB_PD_ALT_MODE_DFP
#define CONFIG_USB_PD_DUAL_ROLE
#define CONFIG_USB_PD_INTERNAL_COMP
#define CONFIG_USB_PD_PORT_COUNT 1
#define CONFIG_USB_PD_TCPC
#define CONFIG_USB_PD_TCPM_STUB
#define CONFIG_USB_PD_VBUS_DETECT_GPIO
#define CONFIG_PD_USE_DAC_AS_REF
#define CONFIG_HW_CRC

#ifndef HAS_TASK_PD_C0 /* PD sniffer mode */
#undef CONFIG_DMA_DEFAULT_HANDLERS
#define CONFIG_USB_PD_TX_PHY_ONLY
/* override the comparator interrupt handler */
#undef CONFIG_USB_PD_RX_COMP_IRQ
#endif

#define CONFIG_ADC
#define CONFIG_BOARD_PRE_INIT
#define CONFIG_CMD_REBOOT_DFU
#define CONFIG_CMD_USB_PD_PE
#define CONFIG_I2C
#define CONFIG_I2C_MASTER
#if defined(BOARD_TWONKIEV2)
#define CONFIG_INA237
#elif defined(BOARD_TWONKIE)
#define CONFIG_INA260
#else /* BOARD_TWINKIE */
#define CONFIG_INA231
#endif
#undef CONFIG_WATCHDOG_HELP
#undef CONFIG_LID_SWITCH
#undef CONFIG_TASK_PROFILING

/* I2C ports configuration */
#define I2C_PORT_MASTER 0

/* USB configuration */
#define CONFIG_USB_PID 0x500A
/* By default, enable all console messages excepted USB */
#define CC_DEFAULT     (CC_ALL & ~CC_MASK(CC_USB))

/*
 * Allow dangerous commands all the time, since we don't have a write protect
 * switch.
 */
#define CONFIG_SYSTEM_UNLOCKED

#ifndef __ASSEMBLER__

struct rx_header;

void sniffer_init(void);

int wait_packet(int pol, uint32_t min_edges, uint32_t timeout_us);

int expect_packet(int pol, uint8_t cmd, uint32_t timeout_us);

uint8_t recording_enable(uint8_t mask);

void trace_packets(void);

void set_trace_mode(int mode);

void sniffer_trace_packet(struct rx_header rx, uint32_t *payload);
void sniffer_trace_reload(void);

/* Timer selection */
#define TIM_CLOCK_MSB  3
#define TIM_CLOCK_LSB 15
#define TIM_ADC       16

#include "gpio_signal.h"

/* ADC signal */
enum adc_channel {
	ADC_CH_CC1_PD = 0,
	ADC_CH_CC2_PD,
	/* Number of ADC channels */
	ADC_CH_COUNT
};

/* USB string indexes */
enum usb_strings {
	USB_STR_DESC = 0,
	USB_STR_VENDOR,
	USB_STR_PRODUCT,
	USB_STR_VERSION,
	USB_STR_SNIFFER,
	USB_STR_CONSOLE_NAME,
	USB_STR_COMMAND_NAME,
	USB_STR_SERIALNO,

	USB_STR_COUNT
};

/* Standard-current Rp */
#define PD_SRC_VNC           PD_SRC_DEF_VNC_MV
#define PD_SRC_RD_THRESHOLD  PD_SRC_DEF_RD_THRESH_MV

/* delay necessary for the voltage transition on the power supply */
#define PD_POWER_SUPPLY_TURN_ON_DELAY  50000 /* us */
#define PD_POWER_SUPPLY_TURN_OFF_DELAY 50000 /* us */

/* Define typical operating power and max power */
#define PD_OPERATING_POWER_MW 15000
#define PD_MAX_POWER_MW       60000
#define PD_MAX_CURRENT_MA     3000
#define PD_MAX_VOLTAGE_MV     20000

#endif /* !__ASSEMBLER__ */

/* USB interface indexes (use define rather than enum to expand them) */
#define USB_IFACE_CONSOLE 0
#define USB_IFACE_VENDOR  1

/* USB endpoint indexes (use define rather than enum to expand them) */
#define USB_EP_CONTROL   0
#define USB_EP_CONSOLE   1
#define USB_EP_COMMAND   2

#ifdef HAS_TASK_SNIFFER
#define USB_EP_SNIFFER   3
#define USB_EP_COUNT     4

#define USB_IFACE_COMMAND 2
#define USB_IFACE_COUNT   3
#else
#define USB_EP_COUNT     3
/* No IFACE_VENDOR for the sniffer */
#define USB_IFACE_COMMAND 1
#define USB_IFACE_COUNT   2
#endif

#if defined(BOARD_TWONKIEV2)
#define INA_SENSE_MOHMS 12
#elif defined(BOARD_TWONKIE)
#define INA_SENSE_MOHMS 2   /* that's mOhms not MOhms */
#else /* Twinkie */
#define INA_SENSE_MOHMS 15
#endif

#endif /* __CROS_EC_BOARD_H */
