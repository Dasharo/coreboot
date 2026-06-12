/* SPDX-License-Identifier: GPL-2.0-only */

#include <soc/ramstage.h>

void mainboard_silicon_init_params(FSP_S_CONFIG *params)
{
	// Enable TCP1 and TCP2 USB-A conversion
	// BIT 0:3 is mapping to PCH XHCI USB2 port
	// BIT 4:5 is reserved
	// BIT 6 is orientational
	// BIT 7 is enable
	params->EnableTcssCovTypeA[1] = 0x86;
	params->EnableTcssCovTypeA[2] = 0x87;

	// XXX: Enabling C10 reporting causes system to constantly enter and
	// exit opportunistic suspend when idle.
	params->PchEspiHostC10ReportEnable = 1;

	// Disable PCIe power gating on the RP that does not have a clock request
	// connected. Otherwise, the connected device will fail after exiting D3.
	params->PciePowerGating[9] = false;
	params->PcieClockGating[9] = false;
}
