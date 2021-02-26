/* SPDX-License-Identifier: GPL-2.0-only */

#ifndef VARIANTS_H
#define VARIANTS_H

#include <soc/gpio.h>
#include <soc/meminit.h>
#include <stdint.h>


/* The next set of functions return the gpio table and fill in the number of
 * entries for each table. */
const struct pad_config *variant_gpio_table(size_t *num);
const struct pad_config *variant_early_gpio_table(size_t *num);
const struct pad_config *variant_sleep_gpio_table(size_t *num);

/* Return memory SKU for the board. */
size_t variant_memory_sku(void);


#endif /* VARIANTS_H */
