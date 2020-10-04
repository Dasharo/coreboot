/* SPDX-License-Identifier: GPL-2.0-only */

#ifndef __BASEBOARD_VARIANTS_H__
#define __BASEBOARD_VARIANTS_H__

#include <soc/cnl_memcfg_init.h>
#include <soc/gpio.h>
#include <vendorcode/google/chromeos/chromeos.h>

/* The next set of functions return the gpio table and fill in the number of
 * entries for each table. */

const struct pad_config *variant_gpio_table(size_t *num);
const struct pad_config *variant_early_gpio_table(size_t *num);

const struct cros_gpio *variant_cros_gpios(size_t *num);

/* Return memory configuration structure. */
const struct cnl_mb_cfg *variant_memcfg_config(void);

/* Seed the NHLT tables with the board specific information. */
struct nhlt;
void variant_nhlt_init(struct nhlt *nhlt);

#endif /*__BASEBOARD_VARIANTS_H__ */
