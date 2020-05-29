/* SPDX-License-Identifier: GPL-2.0-only */

/*
 * Processor Object
 *
 */
Scope (\_PR) {		/* define processor scope */
	Processor(
		C000,		/* name space name, align with BLDCFG_PROCESSOR_SCOPE_NAME[01] */
		0,		/* Unique number for this processor */
		0x810,		/* PBLK system I/O address !hardcoded! */
		0x06		/* PBLKLEN for boot processor */
		) {
	}

	Processor(
		C001,		/* name space name */
		1,		/* Unique number for this processor */
		0x810,		/* PBLK system I/O address !hardcoded! */
		0x06		/* PBLKLEN for boot processor */
		) {
	}
} /* End _SB scope */
