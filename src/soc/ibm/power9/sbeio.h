/* SPDX-License-Identifier: GPL-2.0-only */

#ifndef __SOC_IBM_POWER9_SBEIO_H
#define __SOC_IBM_POWER9_SBEIO_H

#include <stdint.h>

void write_sbe_scom(uint8_t chip, uint64_t addr, uint64_t data);
uint64_t read_sbe_scom(uint8_t chip, uint64_t addr);

#endif /* __SOC_IBM_POWER9_SBEIO_H */
