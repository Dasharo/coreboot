/* SPDX-License-Identifier: GPL-2.0-or-later */

#include <bootblock_common.h>
#include <northbridge/intel/sandybridge/raminit_native.h>
#include <southbridge/intel/bd82x6x/pch.h>
#include <ec/hp/kbc1126/ec.h>

const struct southbridge_usb_port mainboard_usb_ports[] = {
	{ 1, 1, 0 }, /* back bottom USB port, USB debug */
	{ 1, 1, 0 }, /* back upper USB port */
	{ 1, 1, 1 }, /* eSATA */
	{ 1, 1, 1 }, /* webcam */
	{ 1, 0, 2 },
	{ 1, 0, 2 }, /* bluetooth */
	{ 1, 0, 3 },
	{ 1, 0, 3 }, /* smartcard */
	{ 1, 1, 4 }, /* fingerprint reader */
	{ 1, 1, 4 }, /* WWAN */
	{ 0, 0, 5 },
	{ 1, 0, 5 }, /* docking */
	{ 0, 0, 6 },
	{ 0, 0, 6 },
};

void bootblock_mainboard_early_init(void)
{
	kbc1126_enter_conf();
	kbc1126_mailbox_init();
	kbc1126_kbc_init();
	kbc1126_ec_init();
	kbc1126_pm1_init();
	kbc1126_exit_conf();
	kbc1126_disable4e();
}

void mainboard_get_spd(spd_raw_data *spd, bool id_only)
{
	read_spd(&spd[0], 0x50, id_only);
	read_spd(&spd[2], 0x52, id_only);
}
