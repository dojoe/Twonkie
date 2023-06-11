/* Copyright (c) 2014 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * TI INA219/231/237/260 Current/Power monitor driver.
 */

#include "console.h"
#include "hooks.h"
#include "i2c.h"
#include "system.h"
#include "timer.h"
#include "ina2xx.h"
#include "util.h"

#ifdef CONFIG_INA237

#define INA2XX_REG_RESET      0x00
// We define REG_CONFIG to be the one containing the conversion mode reg;
// in the case of INA237 that's the ADC_CONFIG register, not the CONFIG one
#define INA2XX_REG_CONFIG     0x01
#define INA2XX_REG_CONFIG2    0x00
#define INA2XX_REG_CALIB      0x02
#define INA2XX_REG_SHUNT_VOLT 0x04
#define INA2XX_REG_BUS_VOLT   0x05
#define INA2XX_REG_POWER      0x08
#define INA2XX_REG_CURRENT    0x07
#define INA2XX_REG_MASK       0x0B
#define INA2XX_REG_BOVL       0x0E

#else /* INA219 / INA231 / INA260 */

#ifdef CONFIG_INA260
#define INA2XX_REG_CURRENT    0x01
#else
#define INA2XX_REG_CURRENT    0x04
#endif

#define INA2XX_REG_RESET      0x00
#define INA2XX_REG_CONFIG     0x00
#define INA2XX_REG_SHUNT_VOLT 0x01
#define INA2XX_REG_BUS_VOLT   0x02
#define INA2XX_REG_POWER      0x03
#define INA2XX_REG_CALIB      0x05
#define INA2XX_REG_MASK       0x06
#define INA2XX_REG_BOVL       0x07


#define INA2XX_MASK_EN_LEN         (1 << 0)
#define INA2XX_MASK_EN_APOL        (1 << 1)
#define INA2XX_MASK_EN_OVF         (1 << 2)
#define INA2XX_MASK_EN_CVRF        (1 << 3)
#define INA2XX_MASK_EN_AFF         (1 << 4)
#define INA2XX_MASK_EN_CNVR        (1 << 10)
#define INA2XX_MASK_EN_POL         (1 << 11)
#define INA2XX_MASK_EN_BUL         (1 << 12)
#define INA2XX_MASK_EN_BOL         (1 << 13)
#define INA2XX_MASK_EN_SUL         (1 << 14)
#define INA2XX_MASK_EN_SOL         (1 << 15)

#endif

#define INA2XX_RESET 0x8000

/* Console output macros */
#define CPRINTS(format, args...) cprints(CC_USBCHARGE, format, ## args)

uint8_t ina2xx_addresses[INA2XX_MAX_DEV] = {
	INA2XX_I2C_ADDR(0), INA2XX_I2C_ADDR(1), INA2XX_I2C_ADDR(2), INA2XX_I2C_ADDR(3),
	INA2XX_I2C_ADDR(4), INA2XX_I2C_ADDR(5), INA2XX_I2C_ADDR(6), INA2XX_I2C_ADDR(7),
};

static uint16_t ina2xx_modes[INA2XX_MAX_DEV];

static uint16_t ina2xx_read(uint8_t idx, uint8_t reg)
{
	int res;
	int val;
	uint16_t addr = ina2xx_addresses[idx];

	res = i2c_read16(I2C_PORT_MASTER, addr | I2C_FLAG_BIG_ENDIAN, reg, &val);
	if (res) {
		CPRINTS("INA2XX I2C read failed");
		return 0x0bad;
	}
	return val;
}

static uint32_t __attribute__((unused)) ina2xx_read24(uint8_t idx, uint8_t reg)
{
	int res;
	int val;
	uint16_t addr = ina2xx_addresses[idx];

	res = i2c_read24(I2C_PORT_MASTER, addr | I2C_FLAG_BIG_ENDIAN, reg, &val);
	if (res) {
		CPRINTS("INA2XX I2C read failed");
		return 0x0bad;
	}
	return val;
}

static int ina2xx_write(uint8_t idx, uint8_t reg, uint16_t val)
{
	int res;
	uint16_t addr = ina2xx_addresses[idx];

	res = i2c_write16(I2C_PORT_MASTER, addr | I2C_FLAG_BIG_ENDIAN, reg, val);
	if (res)
		CPRINTS("INA2XX I2C write failed");
	return res;
}

int ina2xx_init(uint8_t idx, uint32_t config, uint16_t calib)
{
	int res;

	// Save off the desired conversion mode. If the user wants one-shot mode
	// put the device into power-down state initially.
	ina2xx_modes[idx] = config;

	// Reset the chip, then write the config values
	res = ina2xx_write(idx, INA2XX_REG_RESET, INA2XX_RESET);
	res |= ina2xx_write(idx, INA2XX_REG_CONFIG, (config & INA2XX_CONFIG_MODE_CONT) ? config : 0);
#ifdef INA2XX_HAS_CONFIG2_REG
	res |= ina2xx_write(idx, INA2XX_REG_CONFIG2, config >> 16);
#endif
#ifdef INA2XX_HAS_CALIB_REG
	/* TODO(crosbug.com/p/29730): assume 1mA/LSB, revisit later */
	res |= ina2xx_write(idx, INA2XX_REG_CALIB, calib);
#endif

	return res;
}

int ina2xx_set_vbus_ov_limit(uint8_t idx, int16_t limit)
{
	int res = ina2xx_write(idx, INA2XX_REG_BOVL, limit);
#ifndef CONFIG_INA237
	res |= ina2xx_write(idx, INA2XX_REG_MASK,
			    (limit < INA2XX_OV_LIMIT_DISABLE) ? INA2XX_MASK_EN_BOL : 0);
#endif
	return res;
}

int ina2xx_trigger(uint8_t idx)
{
	return ina2xx_write(idx, INA2XX_REG_CONFIG, ina2xx_modes[idx]);
}

int ina2xx_powerdown(uint8_t idx)
{
	return ina2xx_write(idx, INA2XX_REG_CONFIG, 0);
}

int ina2xx_get_voltage(uint8_t idx)
{
	uint16_t bv = ina2xx_read(idx, INA2XX_REG_BUS_VOLT);
	return INA2XX_BUS_MV((int)bv);
}

int ina2xx_get_current(uint8_t idx)
{
	int16_t curr = ina2xx_read(idx, INA2XX_REG_CURRENT);
	return INA2XX_CURR_MA((int)curr);
}

int ina2xx_get_power(uint8_t idx)
{
#ifdef INA2XX_24BIT_POWER
	uint32_t pow = ina2xx_read24(idx, INA2XX_REG_POWER);
#else
	uint16_t pow = ina2xx_read(idx, INA2XX_REG_POWER);
#endif
	return INA2XX_POW_MW((int)pow);
}

#ifdef CONFIG_CMD_INA
static void ina2xx_dump(uint8_t idx)
{
	uint16_t cfg = ina2xx_read(idx, INA2XX_REG_CONFIG);
	int16_t sv = ina2xx_read(idx, INA2XX_REG_SHUNT_VOLT);
	uint16_t bv = ina2xx_read(idx, INA2XX_REG_BUS_VOLT);
#ifdef INA2XX_24BIT_POWER
	uint32_t pow = ina2xx_read24(idx, INA2XX_REG_POWER);
#else
	uint32_t pow = ina2xx_read(idx, INA2XX_REG_POWER);
#endif
	int16_t curr = ina2xx_read(idx, INA2XX_REG_CURRENT);
	uint16_t calib = ina2xx_read(idx, INA2XX_REG_CALIB);
	uint16_t mask = ina2xx_read(idx, INA2XX_REG_MASK);
	uint16_t alert = ina2xx_read(idx, INA2XX_REG_BOVL);

	ccprintf("Configuration: %04x\n", cfg);
	ccprintf("Shunt voltage: %04x => %d uV\n", sv,
						   INA2XX_SHUNT_UV((int)sv));
	ccprintf("Bus voltage  : %04x => %d mV\n", bv,
						   INA2XX_BUS_MV((int)bv));
	ccprintf("Power        : %06x => %d mW\n", pow,
						   INA2XX_POW_MW((int)pow));
	ccprintf("Current      : %04x => %d mA\n", curr,
						   INA2XX_CURR_MA((int)curr));
	ccprintf("Calibration  : %04x\n", calib);
	ccprintf("Mask/Enable  : %04x\n", mask);
	ccprintf("Alert limit  : %04x\n", alert);
}

/*****************************************************************************/
/* Console commands */

static int command_ina(int argc, char **argv)
{
	char *e;
	int idx;
	uint16_t val;

	if (argc < 2)
		return EC_ERROR_PARAM_COUNT;

	idx = strtoi(argv[1], &e, 10);
	if (*e)
		return EC_ERROR_PARAM1;

	if (2 == argc) { /* dump all registers */
		ina2xx_dump(idx);
		return EC_SUCCESS;
	} else if (4 == argc) {
		val = strtoi(argv[3], &e, 16);
		if (*e)
			return EC_ERROR_PARAM3;

		if (!strcasecmp(argv[2], "config")) {
			ina2xx_write(idx, INA2XX_REG_CONFIG, val);
		} else if (!strcasecmp(argv[2], "calib")) {
			ina2xx_write(idx, INA2XX_REG_CALIB, val);
		} else if (!strcasecmp(argv[2], "mask")) {
			ina2xx_write(idx, INA2XX_REG_MASK, val);
		} else if (!strcasecmp(argv[2], "alert")) {
			ina2xx_write(idx, INA2XX_REG_BOVL, val);
		} else { /* read one register */
			ccprintf("Invalid register: %s\n", argv[1]);
			return EC_ERROR_INVAL;
		}
		return EC_SUCCESS;
	}

	return EC_ERROR_INVAL;
}
DECLARE_CONSOLE_COMMAND(ina, command_ina,
			"<index> [config|calib|mask|alert <val>]",
			"INA2XX power/current sensing");
#endif
