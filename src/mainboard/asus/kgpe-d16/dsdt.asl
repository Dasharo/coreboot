/* SPDX-License-Identifier: GPL-2.0-only */

#include <acpi/acpi.h>
DefinitionBlock (
		"DSDT.AML",	/* Output filename */
		"DSDT",		/* Signature */
		0x02,		/* DSDT Revision, needs to be 2 or higher for 64bit */
		OEM_ID,
		ACPI_TABLE_CREATOR,
		0x00000001	/* OEM Revision */
		)
{
	#include <acpi/dsdt_top.asl>
}
