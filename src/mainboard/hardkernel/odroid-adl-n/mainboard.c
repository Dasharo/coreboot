/* SPDX-License-Identifier: GPL-2.0-only */

#include <device/device.h>
#include <soc/ramstage.h>
#include <soc/gpio.h>
#include <gpio.h>
#include <intelblocks/cse.h>
#include <smbios.h>
#include <string.h>

void mainboard_silicon_init_params(FSP_S_CONFIG *params)
{
	memset(params->PcieRpEnableCpm, 0, sizeof(params->PcieRpEnableCpm));
	memset(params->PcieRpPmSci, 0, sizeof(params->PcieRpPmSci));

	params->PcieRpEnableCpm[2] = 1; // LAN1
	params->PcieRpEnableCpm[3] = 1; // LAN2
	params->PcieRpEnableCpm[6] = 1; // ASMedia PCIe to SATA
	if (!CONFIG(ODROID_H4_NETCARD_SUPPORT))
		params->PcieRpEnableCpm[8] = 1; // NVMe

	// Max payload 256B
	memset(params->PcieRpMaxPayload, 1, sizeof(params->PcieRpMaxPayload));

	// I2C
	params->PchSerialIoI2cSdaPinMux[0] = 0x1947c404; // GPP_H4
	params->PchSerialIoI2cSclPinMux[0] = 0x1947a405; // GPP_H5
	params->PchSerialIoI2cSdaPinMux[1] = 0x1947c606; // GPP_H6
	params->PchSerialIoI2cSclPinMux[1] = 0x1947a607; // GPP_H7

	params->CnviRfResetPinMux = 0;
	params->CnviClkreqPinMux = 0;
}

#if CONFIG(GENERATE_SMBIOS_TABLES)
static int mainboard_smbios_data(struct device *dev, int *handle, unsigned long *current)
{
	int len = 0;

	len += cse_write_smbios_type14(handle, current);

	return len;
}
#endif

static void mainboard_enable(struct device *dev)
{
#if CONFIG(GENERATE_SMBIOS_TABLES)
	dev->ops->get_smbios_data = mainboard_smbios_data;
#endif
}

struct chip_operations mainboard_ops = {
	.enable_dev = mainboard_enable,
};
