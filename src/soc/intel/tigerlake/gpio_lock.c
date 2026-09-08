/* SPDX-License-Identifier: GPL-2.0-only */

#include <gpio.h>
#include <intelblocks/gpio.h>
#include <types.h>

/*
 * GPP_B13 drives PLTRST# as its native function 1 on Tiger Lake PCH-LP as
 * well as on PCH-H. Locking the pad configuration keeps system software from
 * switching the pad to GPIO mode and pulsing it, which would reset a discrete
 * TPM without restarting the measured boot chain.
 */
static const struct gpio_lock_config pltrst_pad[] = {
	{ .pad = GPP_B13, .lock_action = GPIO_LOCK_CONFIG },
};

const struct gpio_lock_config *soc_gpio_lock_config(size_t *num)
{
	*num = ARRAY_SIZE(pltrst_pad);
	return pltrst_pad;
}
