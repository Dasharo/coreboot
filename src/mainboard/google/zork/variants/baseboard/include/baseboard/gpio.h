/* SPDX-License-Identifier: GPL-2.0-or-later */

#ifndef __BASEBOARD_GPIO_H__
#define __BASEBOARD_GPIO_H__

#ifndef __ACPI__
#include <soc/gpio.h>
#include <platform_descriptors.h>

#if CONFIG(BOARD_GOOGLE_BASEBOARD_TREMBYLE)
#define EC_IN_RW_OD		GPIO_130
#else
#define EC_IN_RW_OD		GPIO_11
#endif

/* SPI Write protect */
#define CROS_WP_GPIO		GPIO_137
#define GPIO_EC_IN_RW		EC_IN_RW_OD

#endif /* _ACPI__ */

/* These define the GPE, not the GPIO. */
#define EC_SCI_GPI		3	/* eSPI system event -> GPE 3 */
#define EC_WAKE_GPI		15	/* AGPIO 24 -> GPE 15 */

/* EC sync irq */
#define EC_SYNC_IRQ		31

#endif /* __BASEBOARD_GPIO_H__ */
