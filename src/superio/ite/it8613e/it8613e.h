/* SPDX-License-Identifier: GPL-2.0-or-later */

#ifndef SUPERIO_ITE_IT8613E_H
#define SUPERIO_ITE_IT8613E_H

/*
 * IT8613 supports 2 clock inputs: PCICLK and CLKIN. Multiple registers need
 * to be set to choose proper source. PCICLK is required for LPC.
 *
 * In the table below PD means pull-down, X - don't care.
 *
 * |-------------------------------------------------------------------|
 * | CLKIN  | PCICLK | LDN7\   | GBL\    | LDN7\   | LDN7\   | GBL\    |
 * |        |        |  71h[3] |  23h[3] |  2Dh[2] |  2Dh[1] |  23h[0] |
 * |--------+--------+---------+---------+---------+---------+---------|
 * |   PD   | 33 MHz |    X    |    0    |    0    |    0    |    0    |
 * |   PD   | 24 MHz |    1    |    1    |    X    |    0    |    1    |
 * |   PD   | 25 MHz |    X    |    0    |    1    |    0    |    0    |
 * | 24 MHz |    X   |    0    |    1    |    X    |    0    |    1    |
 * | 48 MHz |    X   |    0    |    1    |    X    |    0    |    0    |
 * |-------------------------------------------------------------------|
 *
 */

#define IT8613E_SP1  0x01 /* Com1 */
#define IT8613E_EC   0x04 /* Environment controller */
#define IT8613E_KBCK 0x05 /* PS/2 keyboard */
#define IT8613E_KBCM 0x06 /* PS/2 mouse */
#define IT8613E_GPIO 0x07 /* GPIO */
#define IT8613E_CIR  0x0a /* Consumer Infrared */

/* GPIO Polarity Select: 1: Inverting, 0: Non-inverting */
#define GPIO_REG_POLARITY(x)	(0xb0 + (x))
#define   GPIO_POL_NO_INVERT	0
#define   GPIO_POL_INVERT	1

/* GPIO Internal Pull-up: 1: Enable, 0: Disable */
#define GPIO_REG_PULLUP(x)	(0xb8 + (x))
#define   GPIO_PULLUP_DIS		0
#define   GPIO_PULLUP_EN		1

/* GPIO Function Select: 1: Simple I/O, 0: Alternate function */
#define GPIO_REG_ENABLE(x)	(0xc0 + (x))
#define   GPIO_ALT_FN		0
#define   GPIO_SIMPLE_IO	1

/* GPIO Mode: 0: input mode, 1: output mode */
#define GPIO_REG_OUTPUT(x)	(0xc8 + (x))
#define   GPIO_INPUT_MODE	0
#define   GPIO_OUTPUT_MODE	1

#endif /* SUPERIO_ITE_IT8613E_H */
