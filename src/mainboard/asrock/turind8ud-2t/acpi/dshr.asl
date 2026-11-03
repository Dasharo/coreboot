/* SPDX-License-Identifier: GPL-2.0-only */

Scope (\_SB)
{
	/* BMC sensor numbers, from the SDR */
	#define DSHR_IPMI_CPU_TEMPS	0x2a			/* TEMP_CPU */
	#define DSHR_IPMI_BOARD_TEMPS	0x50, 0x51, 0x59	/* TEMP_MB1-2, TEMP_LAN */
	#define DSHR_IPMI_CHASSIS_TEMPS	0x55, 0x56		/* TEMP_OUTLET1-2 */
	/* FAN1-FAN6, FAN1_2-FAN6_2 */
	#define DSHR_IPMI_CHASSIS_FANS	0x11, 0x12, 0x13, 0x14, 0x15, 0x16, \
					0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e
	#include <drivers/ipmi/acpi/dshr.asl>
}
