/* SPDX-License-Identifier: GPL-2.0-only */

Scope (\_SB)
{
	/* BMC sensor numbers, from the SDR */
	#define DSHR_IPMI_CPU_TEMPS	0x01			/* CPU0_TEMP */
	/* VR_VCORE_0/1_P0_Tj, VR_VDDIO_P0_Tj, M2_AMB_TEMP, HIC_TEMP_0-2 */
	#define DSHR_IPMI_BOARD_TEMPS	0x0e, 0x10, 0x11, 0x17, 0x08, 0x09, 0x0a
	#define DSHR_IPMI_CPU_FANS	0xb8			/* CPU0_FAN */
	/* SYS_FAN1-SYS_FAN5 */
	#define DSHR_IPMI_CHASSIS_FANS	0xba, 0xbb, 0xbc, 0xbd, 0xbe
	#include <drivers/ipmi/acpi/dshr.asl>
}
