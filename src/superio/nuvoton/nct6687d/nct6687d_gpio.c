/* SPDX-License-Identifier: GPL-2.0-or-later */

#include <arch/io.h>

#include "nct6687d_ec.h"
#include "nct6687d_gpio.h"
#include "nct6687d_hwm.h"

#define NCT6687D_MAX_GPIOS	16

void nct6687d_gpio_set(uint16_t hwm_iobase, u8 gpio_num, u8 gpio_state,
		       enum nct6687d_gpio_type type)
{
	uint8_t group = gpio_num / 10;
	uint8_t pin = gpio_num % 10;
	uint8_t idx;
	uint8_t reg_offset;
	uint8_t reg_val;

	if (pin > 7)
		return;

	if (group > 9)
		return;

	/* Find unused GPIO index or currently selected GPIO */
	for (idx = 0; idx < NCT6687D_MAX_GPIOS; idx++) {
		reg_val = nct6687d_ec_read_page(hwm_iobase, 2, GPO_SEL_REG(idx) & 0xff);
		if (reg_val == 0xff || reg_val == ((group << 4) | pin))
			break;
	}

	if (idx == NCT6687D_MAX_GPIOS)
		return;

	if (reg_val == 0xff) {
		reg_val = (group << 4) | pin;
		nct6687d_ec_write_page(hwm_iobase, 2, GPO_SEL_REG(idx) & 0xff, reg_val);
	}

	/* MSB registers are first, so we have to offset the bits */
	reg_offset = idx < 8 ? 1 : 0;
	reg_val = idx < 8 ? (1 << idx) : (1 << (idx - 8));

	if (type == NCT6687D_GPIO_PUSHPULL)
		nct6687d_ec_and_or_page(hwm_iobase, 2, GPO_TYPE_HI_REG + reg_offset,
					~reg_val, reg_val);
	else
		nct6687d_ec_and_or_page(hwm_iobase, 2, GPO_TYPE_HI_REG + reg_offset,
					~reg_val, 0);

	if (gpio_state)
		nct6687d_ec_and_or_page(hwm_iobase, 2, GPO_DATA_HI_REG + reg_offset,
					~reg_val, reg_val);
	else
		nct6687d_ec_and_or_page(hwm_iobase, 2, GPO_DATA_HI_REG + reg_offset,
					~reg_val, 0);
}
