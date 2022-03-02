/* SPDX-License-Identifier: GPL-2.0-or-later */

#include <console/console.h>
#include <cpu/x86/smm.h>
#include <ec/clevo/it5570/ec.h>
#include <intelblocks/smihandler.h>

void mainboard_smi_espi_handler(void)
{
	printk(BIOS_DEBUG, "Mainboard SMI eSPI handler\n");
}

void mainboard_smi_sleep(u8 slp_typ)
{
	printk(BIOS_DEBUG, "Mainboard SMI sleep handler: %02x\n", slp_typ);
}

int mainboard_smi_apmc(u8 apmc)
{
	printk(BIOS_DEBUG, "Mainboard SMI APMC handler: %02x\n", apmc);
	clevo_it5570_ec_smi_apmc(apmc);
	return 0;
}
