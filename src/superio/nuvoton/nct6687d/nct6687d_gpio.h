/* SPDX-License-Identifier: GPL-2.0-or-later */

/* Both NCT6687DD and NCT6687DF package variants are supported. */

#ifndef SUPERIO_NUVOTON_NCT6687D_GPIO_H
#define SUPERIO_NUVOTON_NCT6687D_GPIO_H

#include <types.h>

enum nct6687d_gpio_type {
	NCT6687D_GPIO_OPENDRAIN,
	NCT6687D_GPIO_PUSHPULL
};

void nct6687d_gpio_set(uint16_t hwm_iobase, u8 gpio_num, u8 gpio_state,
		       enum nct6687d_gpio_type type);

#endif /* SUPERIO_NUVOTON_NCT6687D_GPIO_H */
