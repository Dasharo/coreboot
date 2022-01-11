/* SPDX-License-Identifier: GPL-2.0-only */

#ifndef SR5650_CHIP_H
#define SR5650_CHIP_H

#include <stdint.h>

/* Member variables are defined in Config.lb. */
struct southbridge_amd_sr5650_config
{
	u8 gpp1_configuration;		/* The configuration of General Purpose Port. */
	u8 gpp2_configuration;		/* The configuration of General Purpose Port. */
	u8 gpp3a_configuration;		/* The configuration of General Purpose Port. */
	u16 port_enable;		/* Which port is enabled? GPP(2,3,4,5,6,7,9,10,11,12,13) */
	u32 pcie_settling_time;	/* How long to wait after link training for PCI-e devices to
					 * initialize before probing PCI-e busses (in microseconds).
					 */
};

/*
 * Porting.h included an open #pragma pack(1) that broke things in hard to debug
 * ways. This struct is one of the examples, and while it won't catch all
 * problems caused by that #pragma, hopefully it can reduce time wasted on
 * debugging in wrong places.
 */
_Static_assert(sizeof(struct southbridge_amd_sr5650_config) == 12,
               "check for bad #pragmas");

#endif /* SR5650_CHIP_H */
