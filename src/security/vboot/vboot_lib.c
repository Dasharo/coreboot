/*
 * This file is part of the coreboot project.
 *
 * Copyright 2024 3mdeb
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <console/console.h>
#include <console/vtxprintf.h>
#include <vb2_api.h>

/*
 * vboot callbacks implemented by coreboot -- necessary for making general API
 * calls when CONFIG(VBOOT_LIB) is enabled.  For callbacks specific to verstage
 * CONFIG(VBOOT), please see vboot_logic.c.
 */
void vb2ex_printf(const char *func, const char *fmt, ...)
{
	va_list args;

	if (func)
		printk(BIOS_INFO, "VB2:%s() ", func);

	va_start(args, fmt);
	vprintk(BIOS_INFO, fmt, args);
	va_end(args);
}

void vb2ex_abort(void)
{
	die("vboot has aborted execution; exit\n");
}
