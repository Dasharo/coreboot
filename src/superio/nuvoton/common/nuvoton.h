/* SPDX-License-Identifier: GPL-2.0-or-later */

#ifndef SUPERIO_NUVOTON_COMMON_PRE_RAM_H
#define SUPERIO_NUVOTON_COMMON_PRE_RAM_H

#include <device/pnp_type.h>
#include <stdint.h>

void nuvoton_pnp_enter_conf_state(pnp_devfn_t dev);
void nuvoton_pnp_exit_conf_state(pnp_devfn_t dev);
void nuvoton_enable_serial(pnp_devfn_t dev, u16 iobase);
u8 nuvoton_read_register(pnp_devfn_t dev, u8 read_reg);

#endif /* SUPERIO_NUVOTON_COMMON_PRE_RAM_H */
