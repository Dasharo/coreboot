/* SPDX-License-Identifier: GPL-2.0-or-later */

#ifndef SUPERIO_ITE_IT8625E_H
#define SUPERIO_ITE_IT8625E_H

#define IT8625E_EC_TMP_REG_CNT		    6
#define IT8625E_EC_REG_SMI_MASK_3       0x06
#define  IT8625E_EC_BANK_SEL_MASK	    0x60

static const uint8_t IT8625E_EC_TEMP_ADJUST[] = { 0x56, 0x57, 0x59, 0x5a, 0x90, 0x91 };

#define IT8625E_SP1  0x01 /* Com1 */
#define IT8625E_SP2  0x02 /* Com2 */
#define IT8625E_PP   0x03 /* Parallel port */
#define IT8625E_EC   0x04 /* Environment controller */
#define IT8625E_KBCK 0x05 /* PS/2 keyboard */
#define IT8625E_KBCM 0x06 /* PS/2 mouse */
#define IT8625E_GPIO 0x07 /* GPIO */
#define IT8625E_CIR  0x0a /* Consumer IR */

#endif /* SUPERIO_ITE_IT8625E_H */
