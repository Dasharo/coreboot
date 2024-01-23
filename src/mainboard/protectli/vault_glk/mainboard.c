/* SPDX-License-Identifier: GPL-2.0-only */

#include <arch/io.h>
#include <device/device.h>
#include <fsp/api.h>
#include <pc80/i8254.h>
#include <soc/ramstage.h>

static void mainboard_final(void *chip_info)
{
	if (CONFIG(BEEP_ON_BOOT))
		beep(1500, 100);
}

void mainboard_silicon_init_params(FSP_S_CONFIG *silconfig)
{
	silconfig->C1e = 0x1;
	silconfig->PkgCStateLimit = 0xFE;
	silconfig->CStateAutoDemotion = 0x3;
	silconfig->CStateUnDemotion = 0x3;
	silconfig->PkgCStateDemotion = 0x1;
	silconfig->PkgCStateUnDemotion = 0x1;
	silconfig->Usb30Mode = 0x1;
	silconfig->DspEnable = 0x0;
	silconfig->UsbPdoProgramming = 0x1;

	silconfig->PcieRpHotPlug[0] = 0x0;
	silconfig->PcieRpPmSci[0] = 0x1;
	silconfig->PcieRpTransmitterHalfSwing[0] = 0x0;
	silconfig->PcieRpLtrMaxNonSnoopLatency[0] = 0x1003;
	silconfig->PcieRpLtrMaxSnoopLatency[0] = 0x1003;
	silconfig->PcieRpClkReqSupported[0] = 0x0;
	silconfig->AdvancedErrorReporting[0] = 0x1;
	silconfig->PmeInterrupt[0] = 0x1;

	silconfig->PcieRpHotPlug[1] = 0x0;
	silconfig->PcieRpPmSci[1] = 0x1;
	silconfig->PcieRpTransmitterHalfSwing[1] = 0x0;
	silconfig->PcieRpLtrMaxNonSnoopLatency[1] = 0x1003;
	silconfig->PcieRpLtrMaxSnoopLatency[1] = 0x1003;
	silconfig->PcieRpClkReqSupported[1] = 0x0;
	silconfig->AdvancedErrorReporting[1] = 0x1;
	silconfig->PmeInterrupt[1] = 0x1;

	silconfig->PcieRpHotPlug[2] = 0x0;
	silconfig->PcieRpPmSci[2] = 0x1;
	silconfig->PcieRpTransmitterHalfSwing[2] = 0x0;
	silconfig->PcieRpLtrMaxNonSnoopLatency[2] = 0x1003;
	silconfig->PcieRpLtrMaxSnoopLatency[2] = 0x1003;
	silconfig->PcieRpClkReqSupported[2] = 0x0;
	silconfig->AdvancedErrorReporting[2] = 0x1;
	silconfig->PmeInterrupt[2] = 0x1;

	silconfig->PcieRpHotPlug[3] = 0x0;
	silconfig->PcieRpPmSci[3] = 0x1;
	silconfig->PcieRpTransmitterHalfSwing[3] = 0x0;
	silconfig->PcieRpLtrMaxNonSnoopLatency[3] = 0x1003;
	silconfig->PcieRpLtrMaxSnoopLatency[3] = 0x1003;
	silconfig->PcieRpClkReqSupported[3] = 0x0;
	silconfig->AdvancedErrorReporting[3] = 0x1;
	silconfig->PmeInterrupt[3] = 0x1;

	silconfig->PcieRpHotPlug[4] = 0x0;
	silconfig->PcieRpPmSci[4] = 0x1;
	silconfig->PcieRpTransmitterHalfSwing[4] = 0x0;
	silconfig->PcieRpLtrMaxNonSnoopLatency[4] = 0x1003;
	silconfig->PcieRpLtrMaxSnoopLatency[4] = 0x1003;
	silconfig->PcieRpClkReqSupported[4] = 0x0;
	silconfig->AdvancedErrorReporting[4] = 0x1;
	silconfig->PmeInterrupt[4] = 0x1;

	silconfig->PcieRpHotPlug[5] = 0x0;
	silconfig->PcieRpPmSci[5] = 0x1;
	silconfig->PcieRpTransmitterHalfSwing[5] = 0x0;
	silconfig->PcieRpLtrMaxNonSnoopLatency[5] = 0x1003;
	silconfig->PcieRpLtrMaxSnoopLatency[5] = 0x1003;
	silconfig->PcieRpClkReqSupported[5] = 0x0;
	silconfig->AdvancedErrorReporting[5] = 0x1;
	silconfig->PmeInterrupt[5] = 0x1;
}

struct chip_operations mainboard_ops = {
	.final = mainboard_final,
};
