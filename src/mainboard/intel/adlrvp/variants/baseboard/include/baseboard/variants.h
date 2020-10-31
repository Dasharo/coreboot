/* SPDX-License-Identifier: GPL-2.0-only */

#ifndef __BASEBOARD_VARIANTS_H__
#define __BASEBOARD_VARIANTS_H__

#include <soc/gpio.h>
#include <soc/meminit.h>
#include <stdint.h>
#include <vendorcode/google/chromeos/chromeos.h>

enum adl_boardid {
	/* ADL-P LPDDR4 RVPs */
	ADL_P_LP4_1 = 0x10,
	ADL_P_LP4_2 = 0x11,
	/* ADL-P DDR4 RVPs */
	ADL_P_DDR4_1 = 0x14,
	ADL_P_DDR4_2 = 0x3F,
};

/* The next set of functions return the gpio table and fill in the number of
 * entries for each table. */
const struct cros_gpio *variant_cros_gpios(size_t *num);
/* Functions to configure GPIO as per variant schematics */
void variant_configure_gpio_pads(void);
void variant_configure_early_gpio_pads(void);

size_t variant_memory_sku(void);
const struct mb_cfg *variant_memory_params(void);
#endif /*__BASEBOARD_VARIANTS_H__ */
