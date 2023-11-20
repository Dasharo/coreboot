/* SPDX-License-Identifier: GPL-2.0-or-later */

#include <arch/mmio.h>
#include <device/device.h>
#include <soc/iomap.h>
#include <soc/ramstage.h>

void mainboard_silicon_init_params(FSP_S_CONFIG *params)
{
	params->UsbPdoProgramming = 0;

	params->PcieRpAcsEnabled[0] = 1;
	params->PcieRpAcsEnabled[1] = 1;
	params->PcieRpAcsEnabled[3] = 1;
	params->PcieRpAcsEnabled[4] = 1;
	params->PcieRpAcsEnabled[5] = 1;
	params->PcieRpAcsEnabled[6] = 1;

	params->PcieRpAdvancedErrorReporting[0] = 1;
	params->PcieRpAdvancedErrorReporting[1] = 1;
	params->PcieRpAdvancedErrorReporting[3] = 1;
	params->PcieRpAdvancedErrorReporting[4] = 1;
	params->PcieRpAdvancedErrorReporting[5] = 1;
	params->PcieRpAdvancedErrorReporting[6] = 1;

	params->PcieRpLtrEnable[0] = 1;
	params->PcieRpLtrEnable[1] = 1;
	params->PcieRpLtrEnable[3] = 1;
	params->PcieRpLtrEnable[4] = 1;
	params->PcieRpLtrEnable[5] = 1;
	params->PcieRpLtrEnable[6] = 1;

	/* Set Max Payload to 256B */
	params->PcieRpMaxPayload[0] = 1;
	params->PcieRpMaxPayload[1] = 1;
	params->PcieRpMaxPayload[3] = 1;
	params->PcieRpMaxPayload[4] = 1;
	params->PcieRpMaxPayload[5] = 1;
	params->PcieRpMaxPayload[6] = 1;

	params->PcieRpSlotImplemented[3] = 1;
	params->PcieRpSlotImplemented[4] = 1;

	params->PcieRpDetectTimeoutMs[1] = 200;
}

static void mainboard_enable(struct device *dev)
{
}

struct chip_operations mainboard_ops = {
	.enable_dev = mainboard_enable,
};
