/* SPDX-License-Identifier: GPL-2.0-or-later */

#include <cpu/x86/smm.h>
#include <ec/google/chromeec/smm.h>
#include <intelblocks/smihandler.h>
#include <baseboard/ec.h>

void mainboard_smi_espi_handler(void)
{
	chromeec_smi_process_events();
}

void mainboard_smi_sleep(u8 slp_typ)
{
	chromeec_smi_sleep(slp_typ, MAINBOARD_EC_S3_WAKE_EVENTS,
			MAINBOARD_EC_S5_WAKE_EVENTS);
}

int mainboard_smi_apmc(u8 apmc)
{
	chromeec_smi_apmc(apmc, MAINBOARD_EC_SCI_EVENTS,
			MAINBOARD_EC_SMI_EVENTS);
	return 0;
}
