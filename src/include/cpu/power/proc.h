/* SPDX-License-Identifier: GPL-2.0-only */

#ifndef __SOC_IBM_POWER9_PROC_H
#define __SOC_IBM_POWER9_PROC_H

#include <arch/byteorder.h>	// PPC_BIT(), PPC_BITMASK()

#define MAX_CHIPS			2

#define MAX_CORES_PER_CHIP		24
#define MAX_CORES_PER_EX		2
#define MAX_QUADS_PER_CHIP		(MAX_CORES_PER_CHIP / 4)
#define MAX_CMES_PER_CHIP		(MAX_CORES_PER_CHIP / MAX_CORES_PER_EX)

#define MCS_PER_PROC			2
#define MCA_PER_MCS			2
#define MCA_PER_PROC			(MCA_PER_MCS * MCS_PER_PROC)
#define DIMMS_PER_MCA			2
#define DIMMS_PER_MCS			(DIMMS_PER_MCA * MCA_PER_MCS)
#define DIMMS_PER_PROC			(DIMMS_PER_MCS * MCS_PER_PROC)

#define I2C_BUSES_PER_CPU		4
#define SPD_I2C_BUS			3
#define FSI_I2C_BUS			8	// one bus to the second CPU

/* cores is a 64-bit map of functional cores of a single chip */
#define IS_EC_FUNCTIONAL(ec, cores)	(!!((cores) & PPC_BIT(ec)))
#define IS_EX_FUNCTIONAL(ex, cores)	(!!((cores) & PPC_BITMASK(2 * (ex), 2 * (ex) + 1)))
#define IS_EQ_FUNCTIONAL(eq, cores)	(!!((cores) & PPC_BITMASK(4 * (eq), 4 * (eq) + 3)))

/*
 * This is for ATTR_PROC_FABRIC_PUMP_MODE == PUMP_MODE_CHIP_IS_GROUP,
 * when chip ID is actually a group ID and "chip ID" field is zero.
 *
 * "nm" means non-mirrored.
 */
#define PROC_BASE_ADDR(chip, msel) (                                                      \
		PPC_PLACE(0x0, 8, 5)   | /* system ID */                                  \
		PPC_PLACE(msel, 13, 2) | /* msel (nm = 0b00/01, m = 0b10, mmio = 0b11) */ \
		PPC_PLACE(chip, 15, 4) | /* group ID */                                   \
		PPC_PLACE(0x0, 19, 3)    /* chip ID */                                    \
	)

#endif /* __SOC_IBM_POWER9_PROC_H */
