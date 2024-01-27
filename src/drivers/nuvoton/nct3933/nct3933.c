/* SPDX-License-Identifier: GPL-2.0-only */

#include <console/console.h>

#include "nct3933.h"

/* Based on NCT3933U datasheet: 
 * https://datasheet.octopart.com/NCT3933U-TR-Nuvoton-datasheet-11773509.pdf
 */

#define NCT3933_OUT1_DAC_REG		0x01
#define NCT3933_OUT2_DAC_REG		0x02
#define NCT3933_OUT3_DAC_REG		0x03

#define NCT3933_WDT_REG			0x04
#define   NCT3933_VER_ID_MASK		(3 << 0)
#define   NCT3933_STEP_SPEED_DELAY_MASK	(3 << 2)
#define   NCT3933_STEP_SPEED_DELAY_10US	(0 << 2)
#define   NCT3933_STEP_SPEED_DELAY_20US	(1 << 2)
#define   NCT3933_STEP_SPEED_DELAY_30US	(2 << 2)
#define   NCT3933_STEP_SPEED_DELAY_40US	(3 << 2)
#define   NCT3933_WDT_DELAY_MASK	(3 << 4)
#define   NCT3933_WDT_DELAY_1400MS	(0 << 4)
#define   NCT3933_WDT_DELAY_2800MS	(1 << 4)
#define   NCT3933_WDT_DELAY_5500MS	(2 << 4)
#define   NCT3933_WDT_DELAY_11000MS	(3 << 4)
#define   NCT3933_WDT_TIMEOUT_STS	(1 << 6)
#define   NCT3933_WDT_EN		(1 << 7)

#define NCT3933_TWOFOLD_OUT_CURRENT_REG	0x05
#define   NCT3933_OUT_CURRENT_X2_EN(x)	(1 << (((x) - 1) * 2))
#define   NCT3933_POWER_SAVING_FN_EN	(1 << 6)

#define NCT3933_VENDOR_ID1_REG			0x5d
#define NCT3933_VENDOR_ID2_REG			0x5e

int nct3933_probe(const struct nct3933_smbus_ops *ops, uint8_t addr)
{
	uint8_t ven_id[2] = { 0 };
	uint8_t version = 0;
	
	if (!addr || !ops || !ops->read_byte)
		return -1;

	ops->read_byte(addr, NCT3933_VENDOR_ID1_REG, &ven_id[0]);
	ops->read_byte(addr, NCT3933_VENDOR_ID2_REG, &ven_id[1]);

	if (ven_id[0] != 0x39 && ven_id[1] != 0x33)
		return -1;


	ops->read_byte(addr, NCT3933_VENDOR_ID1_REG, &version);

	printk(BIOS_DEBUG, "Found Nuvoton NCT3933 ver %d @ 0x%02x\n",
	       version & NCT3933_VER_ID_MASK, addr);

	return 0;
}

int nct3933_set_current_dac(const struct nct3933_smbus_ops *ops, uint8_t addr, uint8_t out_no,
			    uint8_t value)
{
	if (out_no < NCT3933_OUT1_DAC_REG || out_no > NCT3933_OUT3_DAC_REG)
		return -1;

	if (!addr || !ops || !ops->write_byte)
		return -1;

	return ops->write_byte(addr, NCT3933_OUT_DAC_REG(out_no), value);
}

int nct3933_get_current_dac(const struct nct3933_smbus_ops *ops, uint8_t addr, uint8_t out_no,
			    uint8_t *value)
{
	if (out_no < NCT3933_OUT1_DAC_REG || out_no > NCT3933_OUT3_DAC_REG)
		return -1;

	if (!addr || !ops || !ops->read_byte)
		return -1;

	return ops->read_byte(addr, NCT3933_OUT_DAC_REG(out_no), value);
}

int nct3933_set_current_x2(const struct nct3933_smbus_ops *ops, uint8_t addr, uint8_t out_no,
			   bool enable)
{
	int ret;
	uint8_t val;

	if (out_no < NCT3933_OUT1_DAC_REG || out_no > NCT3933_OUT3_DAC_REG)
		return -1;

	if (!addr || !ops || !ops->read_byte || !ops->write_byte)
		return -1;

	ret = ops->read_byte(addr, NCT3933_TWOFOLD_OUT_CURRENT_REG, &val);
	if (ret)
		return ret;

	if (enable)
		val |= NCT3933_OUT_CURRENT_X2_EN(out_no);
	else
		val &= ~NCT3933_OUT_CURRENT_X2_EN(out_no);

	return ops->write_byte(addr, NCT3933_TWOFOLD_OUT_CURRENT_REG, val);
}

int nct3933_set_power_saving(const struct nct3933_smbus_ops *ops, uint8_t addr, uint8_t out_no,
			     bool enable)
{
	int ret;
	uint8_t val;

	if (out_no < NCT3933_OUT1_DAC_REG || out_no > NCT3933_OUT3_DAC_REG)
		return -1;

	if (!addr || !ops || !ops->read_byte || !ops->write_byte)
		return -1;

	ret = ops->read_byte(addr, NCT3933_TWOFOLD_OUT_CURRENT_REG, &val);
	if (ret)
		return ret;

	if (enable)
		val |= NCT3933_POWER_SAVING_FN_EN;
	else
		val &= ~NCT3933_POWER_SAVING_FN_EN;

	return ops->write_byte(addr, NCT3933_TWOFOLD_OUT_CURRENT_REG, val);
}