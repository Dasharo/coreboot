/* SPDX-License-Identifier: GPL-2.0-only */

#ifndef __SOC_IBM_POWER9_MCBIST_H
#define __SOC_IBM_POWER9_MCBIST_H

#include <stdint.h>

void add_scrub(uint8_t chip, int mcs_i, int port_dimm);
void add_fixed_pattern_write(uint8_t chip, int mcs_i, int port_dimm);

void mcbist_execute(uint8_t chip, int mcs_i);
int mcbist_is_done(uint8_t chip, int mcs_i);

#endif /* __SOC_IBM_POWER9_MCBIST_H */
