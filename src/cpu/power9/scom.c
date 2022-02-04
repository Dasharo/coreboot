/* SPDX-License-Identifier: GPL-2.0-or-later */

/* Avoids defining read/write_rscom as a macro */
#define SKIP_SCOM_DEBUG

#include <cpu/power/scom.h>
#include <console/console.h>

extern uint64_t read_xscom(uint8_t chip, uint64_t addr);
extern void write_xscom(uint8_t chip, uint64_t addr, uint64_t data);

extern uint64_t read_sbe_scom(uint8_t chip, uint64_t addr);
extern void write_sbe_scom(uint8_t chip, uint64_t addr, uint64_t data);

/* Start with SBEIO. Change this to FSI if needed. */
static uint64_t (*read_scom_secondary)(uint8_t, uint64_t) = read_sbe_scom;
static void (*write_scom_secondary)(uint8_t, uint64_t, uint64_t) = write_sbe_scom;

void switch_secondary_scom_to_xscom(void)
{
	read_scom_secondary = read_xscom;
	write_scom_secondary = write_xscom;
}

uint64_t read_rscom(uint8_t chip, uint64_t addr)
{
	if (chip == 0)
		return read_xscom(chip, addr);
	else
		return read_scom_secondary(chip, addr);
}

void write_rscom(uint8_t chip, uint64_t addr, uint64_t data)
{
	if (chip == 0)
		write_xscom(chip, addr, data);
	else
		write_scom_secondary(chip, addr, data);
}
