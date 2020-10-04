/* SPDX-License-Identifier: GPL-2.0-or-later */

#ifndef __BASEBOARD_VARIANTS_H__
#define __BASEBOARD_VARIANTS_H__

#include <soc/gpio.h>
#include <stdint.h>

/* The next set of functions return the gpio table and fill in the number of
 * entries for each table. */

const struct pad_config *variant_base_gpio_table(size_t *num);
const struct pad_config *variant_early_gpio_table(size_t *num);
const struct pad_config *variant_sleep_gpio_table(size_t *num);
const struct cros_gpio *variant_cros_gpios(size_t *num);
const struct pad_config *variant_override_gpio_table(size_t *num);

/**
 * Get board's Hardware features as defined in FW_CONFIG
 *
 * @param fw_config	Address where the fw_config is stored.
 * @return 0 on success or negative integer for errors.
 */
int board_info_get_fw_config(uint32_t *fw_config);

/* Return memory configuration structure. */
const struct mb_cfg *variant_memcfg_config(void);

/* Return memory SKU for the variant */
int variant_memory_sku(void);

/**
 * Get data whether memory channel is half-populated or not
 *
 * @return false on boards where memory channel is half-populated, true otherwise.
 */
bool variant_mem_is_half_populated(void);

/* Variant Intel Speed Shift Technology override */
void variant_isst_override(void);

#endif /*__BASEBOARD_VARIANTS_H__ */
