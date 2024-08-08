/* SPDX-License-Identifier: GPL-2.0-only */

#include <device/device.h>
#include <soc/ramstage.h>
#include <soc/gpio.h>
#include <gpio.h>
#include <string.h>

void mainboard_silicon_init_params(FSP_S_CONFIG *params)
{
	memset(params->PcieRpEnableCpm, 0, sizeof(params->PcieRpEnableCpm));
	memset(params->PcieRpPmSci, 0, sizeof(params->PcieRpPmSci));

	params->PcieRpEnableCpm[2] = 1; // LAN1
	params->PcieRpEnableCpm[3] = 1; // LAN2
	params->PcieRpEnableCpm[6] = 1; // ASMedia PCIe to SATA
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

	/* GPP_B8 is EMMC_DET#, active low */
	gpio_input_pullup(GPP_B8);
	params->ScsEmmcEnabled = !gpio_get(GPP_B8);


}
