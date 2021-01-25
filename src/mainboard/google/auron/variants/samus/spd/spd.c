/* SPDX-License-Identifier: GPL-2.0-only */

#include <cbfs.h>
#include <console/console.h>
#include <endian.h>
#include <string.h>
#include <soc/gpio.h>
#include <soc/pei_data.h>
#include <soc/romstage.h>
#include <ec/google/chromeec/ec.h>
#include <mainboard/google/auron/ec.h>
#include <mainboard/google/auron/variant.h>

/* Samus board memory configuration GPIOs */
#define SPD_GPIO_BIT0		69
#define SPD_GPIO_BIT1		68
#define SPD_GPIO_BIT2		67
#define SPD_GPIO_BIT3		65

/* Copy SPD data for on-board memory */
void mainboard_fill_spd_data(struct pei_data *pei_data)
{
	int spd_bits[4] = {
		SPD_GPIO_BIT0,
		SPD_GPIO_BIT1,
		SPD_GPIO_BIT2,
		SPD_GPIO_BIT3
	};
	int spd_gpio[4];
	int spd_index;

	spd_gpio[0] = get_gpio(spd_bits[0]);
	spd_gpio[1] = get_gpio(spd_bits[1]);
	spd_gpio[2] = get_gpio(spd_bits[2]);
	spd_gpio[3] = get_gpio(spd_bits[3]);

	spd_index = (spd_gpio[3] << 3) | (spd_gpio[2] << 2) |
		(spd_gpio[1] << 1) | spd_gpio[0];

	printk(BIOS_DEBUG, "SPD: index %d (GPIO%d=%d GPIO%d=%d "
	       "GPIO%d=%d GPIO%d=%d)\n", spd_index,
	       spd_bits[3], spd_gpio[3], spd_bits[2], spd_gpio[2],
	       spd_bits[1], spd_gpio[1], spd_bits[0], spd_gpio[0]);

	fill_spd_for_index(pei_data->spd_data[0][0], spd_index);

	/* Assume same memory in both channels */
	memcpy(pei_data->spd_data[1][0], pei_data->spd_data[0][0], SPD_LEN);
}
