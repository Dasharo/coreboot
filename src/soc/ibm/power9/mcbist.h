/* SPDX-License-Identifier: GPL-2.0-only */

#ifndef __SOC_IBM_POWER9_MCBIST_H
#define __SOC_IBM_POWER9_MCBIST_H

void add_scrub(int mcs_i, int port_dimm);
void add_fixed_pattern_write(int mcs_i, int port_dimm);

void mcbist_execute(int mcs_i);
int mcbist_is_done(int mcs_i);

#endif /* __SOC_IBM_POWER9_MCBIST_H */
