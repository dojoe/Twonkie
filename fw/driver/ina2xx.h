/* Copyright (c) 2014 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * TI INA219/231/237/260 Current/Power monitor driver.
 */

#ifndef __CROS_EC_INA2XX_H
#define __CROS_EC_INA2XX_H

#ifdef CONFIG_INA237

#define INA2XX_CONFIG_MODE_MASK     (15 << 12)
#define INA2XX_CONFIG_MODE_PWRDWN   (0 << 12)
#define INA2XX_CONFIG_MODE_SHUNT    (1 << 13)
#define INA2XX_CONFIG_MODE_BUS      (1 << 12)
#define INA2XX_CONFIG_MODE_TRG      (0 << 15)
#define INA2XX_CONFIG_MODE_CONT     (1 << 15)

/* Conversion time for bus and shunt in micro-seconds */
enum ina2xx_conv_time {
	/* Using INA231 terms here for compatibility and
	 * mapping to the closest INA237 value */
	INA2XX_CONV_TIME_140  = 0x02,
	INA2XX_CONV_TIME_204  = 0x02,
	INA2XX_CONV_TIME_332  = 0x03,
	INA2XX_CONV_TIME_588  = 0x04,
	INA2XX_CONV_TIME_1100 = 0x05,
	INA2XX_CONV_TIME_2116 = 0x06,
	INA2XX_CONV_TIME_4156 = 0x07,
	INA2XX_CONV_TIME_8244 = 0x07,
};
#define INA2XX_CONV_TIME_MASK 0x7
#define INA2XX_CONFIG_SHUNT_CONV_TIME(t) ((t) << 6)
#define INA2XX_CONFIG_BUS_CONV_TIME(t)   ((t) << 9)

#define INA2XX_CONFIG_AVG_1        (0 << 0)
#define INA2XX_CONFIG_AVG_4        (1 << 0)
#define INA2XX_CONFIG_AVG_16       (2 << 0)
#define INA2XX_CONFIG_AVG_64       (3 << 0)
#define INA2XX_CONFIG_AVG_128      (4 << 0)
#define INA2XX_CONFIG_AVG_256      (5 << 0)
#define INA2XX_CONFIG_AVG_512      (6 << 0)
#define INA2XX_CONFIG_AVG_1024     (7 << 0)

#else /* INA219 / INA231 / INA260 */

#define INA2XX_CONFIG_MODE_MASK     (7 << 0)
#define INA2XX_CONFIG_MODE_PWRDWN   (0 << 0)
#define INA2XX_CONFIG_MODE_SHUNT    (1 << 0)
#define INA2XX_CONFIG_MODE_BUS      (1 << 1)
#define INA2XX_CONFIG_MODE_TRG      (0 << 2)
#define INA2XX_CONFIG_MODE_CONT     (1 << 2)

/* Conversion time for bus and shunt in micro-seconds */
enum ina2xx_conv_time {
	INA2XX_CONV_TIME_140  = 0x00,
	INA2XX_CONV_TIME_204  = 0x01,
	INA2XX_CONV_TIME_332  = 0x02,
	INA2XX_CONV_TIME_588  = 0x03,
	INA2XX_CONV_TIME_1100 = 0x04,
	INA2XX_CONV_TIME_2116 = 0x05,
	INA2XX_CONV_TIME_4156 = 0x06,
	INA2XX_CONV_TIME_8244 = 0x07,
};
#define INA2XX_CONV_TIME_MASK 0x7
#define INA2XX_CONFIG_SHUNT_CONV_TIME(t) ((t) << 3)
#define INA2XX_CONFIG_BUS_CONV_TIME(t)   ((t) << 6)

#define INA2XX_CONFIG_AVG_1        (0 << 9)
#define INA2XX_CONFIG_AVG_4        (1 << 9)
#define INA2XX_CONFIG_AVG_16       (2 << 9)
#define INA2XX_CONFIG_AVG_64       (3 << 9)
#define INA2XX_CONFIG_AVG_128      (4 << 9)
#define INA2XX_CONFIG_AVG_256      (5 << 9)
#define INA2XX_CONFIG_AVG_512      (6 << 9)
#define INA2XX_CONFIG_AVG_1024     (7 << 9)

#endif

#define INA2XX_OV_LIMIT_DISABLE    0x7FFF
#define INA2XX_UV_LIMIT_DISABLE    -0x8000

#define INA2XX_MAX_DEV 8
#define INA2XX_HAS_CALIB_REG

#if (defined(CONFIG_INA231) + defined(CONFIG_INA219) + defined(CONFIG_INA237) + defined(CONFIG_INA260)) > 1
#error CONFIG_INA231, CONFIG_INA219, CONFIG_INA237 or CONFIG_INA260 are mutually exclusive
#endif

#ifdef CONFIG_INA231

/* Calibration value to get current LSB = 1mA */
#define INA2XX_CALIB_1MA(rsense_mohm) (5120/(rsense_mohm))
/* Bus voltage: mV per LSB */
#define INA2XX_BUS_MV(reg) ((reg) * 125 / 100)
#define INA2XX_MV_BUS(val) ((val) * 100 / 125)
/* Shunt voltage: uV per LSB */
#define INA2XX_SHUNT_UV(reg) ((reg) * 25 / 10)
/* Shunt current: mA per LSB */
#define INA2XX_CURR_MA(reg) (reg)
/* Power LSB: mW per current LSB */
#define INA2XX_POW_MW(reg) ((reg) * 25 * 1/*Current mA/LSB*/)

#elif defined(CONFIG_INA260)

#define INA2XX_CALIB_1MA(rsense_mohm) 0  /* INA260 is internally calibrated */
#define INA2XX_BUS_MV(reg) ((reg) * 125 / 100)
#define INA2XX_MV_BUS(val) ((val) * 100 / 125)
#define INA2XX_SHUNT_UV(reg) ((reg) * 25 / 10)
#define INA2XX_CURR_MA(reg) ((reg) * 125 / 100)
#define INA2XX_POW_MW(reg) ((reg) * 10)

#undef INA2XX_HAS_CALIB_REG

#elif defined(CONFIG_INA237)

#define INA2XX_CALIB_1MA(rsense_mohm) ((8192*(rsense_mohm))/10)
#define INA2XX_BUS_MV(reg) ((reg) * 3125 / 1000)
#define INA2XX_MV_BUS(val) ((val) * 1000 / 3125)
#define INA2XX_SHUNT_UV(reg) ((reg) * 5)
#define INA2XX_CURR_MA(reg) (reg)
#define INA2XX_POW_MW(reg) ((reg) * 2 / 10)

#define INA2XX_HAS_CONFIG2_REG
#define INA2XX_24BIT_POWER

#else /* CONFIG_INA219 */

#define INA2XX_CALIB_1MA(rsense_mohm) (40960/(rsense_mohm))
#define INA2XX_BUS_MV(reg) ((reg) / 2)
#define INA2XX_MV_BUS(val) ((val) * 2)
#define INA2XX_SHUNT_UV(reg) ((reg) * 2)
#define INA2XX_CURR_MA(reg) (reg)
#define INA2XX_POW_MW(reg) ((reg) * 20 * 1/*Current mA/LSB*/)

#endif

/* 8-bit I2C base address */
#define INA2XX_I2C_ADDR(offset) ((0x40 | (offset)) << 1)

extern uint8_t ina2xx_addresses[INA2XX_MAX_DEV];

/* Set measurement parameters */
int ina2xx_init(uint8_t idx, uint32_t config, uint16_t calib);

/* Set up / clear VBUS overvoltage limit. 0x7FFF = disable */
int ina2xx_set_vbus_ov_limit(uint8_t idx, int16_t limit);

/* Trigger one-shot measurement */
int ina2xx_trigger(uint8_t idx);

/* Put INA into power-down state */
int ina2xx_powerdown(uint8_t idx);

/* Return bus voltage in milliVolts */
int ina2xx_get_voltage(uint8_t idx);

/* Return current in milliAmps */
int ina2xx_get_current(uint8_t idx);

/* Return power in milliWatts */
int ina2xx_get_power(uint8_t idx);

#endif /* __CROS_EC_INA2XX_H */
