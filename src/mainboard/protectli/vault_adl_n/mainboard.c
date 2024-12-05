/* SPDX-License-Identifier: GPL-2.0-only */

#include <arch/cpu.h>
#include <console/console.h>
#include <dasharo/options.h>
#include <device/device.h>
#include <pc80/i8254.h>
#include <smbios.h>
#include <soc/ramstage.h>
#include <superio/ite/it8659e/chip.h>
#include <superio/ite/it8659e/it8659e.h>

#include <string.h>

const char *smbios_mainboard_product_name(void)
{
	if (CONFIG(BOARD_PROTECTLI_VP2430)) {
		return "VP2430";
	}

	u32 tmp[13];
	const char *str = "Unknown Processor Name";

	if (cpu_have_cpuid()) {
		int i;
		struct cpuid_result res;
		if (cpu_cpuid_extended_level() >= 0x80000004) {
			int j = 0;
			for (i = 0; i < 3; i++) {
				res = cpuid(0x80000002 + i);
				tmp[j++] = res.eax;
				tmp[j++] = res.ebx;
				tmp[j++] = res.ecx;
				tmp[j++] = res.edx;
			}
			tmp[12] = 0;
			str = (const char *)tmp;
		}
	}

	if (strstr(str, "N100") != NULL)
		return "VP3210";
	else if (strstr(str, "N305") != NULL)
		return "VP3230";
	else
		return CONFIG_MAINBOARD_SMBIOS_PRODUCT_NAME;
}

void mainboard_silicon_init_params(FSP_S_CONFIG *params)
{
	memset(params->PcieRpEnableCpm, 0, sizeof(params->PcieRpEnableCpm));
	memset(params->PcieRpPmSci, 0, sizeof(params->PcieRpPmSci));

	memset(params->CpuPcieRpEnableCpm, 0, sizeof(params->CpuPcieRpEnableCpm));
	memset(params->CpuPcieClockGating, 0, sizeof(params->CpuPcieClockGating));
	memset(params->CpuPciePowerGating, 0, sizeof(params->CpuPciePowerGating));
	memset(params->CpuPcieRpPmSci, 0, sizeof(params->CpuPcieRpPmSci));

	// Max payload 256B
	memset(params->PcieRpMaxPayload, 1, sizeof(params->PcieRpMaxPayload));

	if (CONFIG(BOARD_PROTECTLI_VP32XX)) {
		/*
		 * CLKREQs connected only to RP3 and RP7, but other CLKREQs are
		 * pulled to GND, So it should be fine to enable CPM on all RPs.
		 */
		params->PcieRpEnableCpm[0] = 1;
		params->PcieRpEnableCpm[2] = 1;
		params->PcieRpEnableCpm[4] = 1;
		params->PcieRpEnableCpm[6] = 1;
		params->PcieRpEnableCpm[9] = 1;
	}

	// Enable port reset message on Type-C ports
	params->PortResetMessageEnable[4] = 1;
	params->PortResetMessageEnable[5] = 1;

	/*
	 * Configure AUX bias pads in FPS-S, becuase coreboot would do it too
	 * late and cause the Type-C displays to not work.
	 */
	params->IomTypeCPortPadCfg[0] = 0x09020016; // GPP_A22
	params->IomTypeCPortPadCfg[1] = 0x09020015; // GPP_A21
	params->IomTypeCPortPadCfg[2] = 0x0902000F; // GPP_A15
	params->IomTypeCPortPadCfg[3] = 0x0902000E; // GPP_A14

	// PMC-PD controller
	params->PmcPdEnable = 1;

#if CONFIG(BOARD_PROTECTLI_VP2430)
	// Only available with custom FSP. W/A for unused CKLREQs.
	params->PchPcieClockGating = 0;
	params->PchPciePowerGating = 0;
#endif

	// IOM USB config
	params->PchUsbOverCurrentEnable = 0;
}

static void mainboard_final(void *chip_info)
{
	if (CONFIG(BEEP_ON_BOOT))
		beep(1500, 100);
}

static void set_fan_curve(struct ite_ec_fan_config *fan, uint8_t fan_curve)
{
	switch (fan_curve) {
	case FAN_CURVE_OPTION_OFF:
		fan->mode = FAN_MODE_OFF;
		break;
	case FAN_CURVE_OPTION_PERFORMANCE:
		fan->smart.pwm_start += 20;
		fan->smart.tmp_full -= 10;
		fan->smart.tmp_off -= 10;
		fan->smart.tmp_start -= 10;
		break;
	case FAN_CURVE_OPTION_SILENT:
		/* Do nothing, it is the default */
	default:
		break;
	}
}

static void mainboard_enable(struct device *dev)
{
	struct superio_ite_it8659e_config *ite_cfg;
	struct device *ite_ec = dev_find_slot_pnp(0x2e, IT8659E_EC);
	uint8_t fan_curve = get_fan_curve_option();

	if (ite_ec && ite_ec->chip_info) {
		ite_cfg = (struct superio_ite_it8659e_config *)ite_ec->chip_info;
		set_fan_curve(&ite_cfg->ec.fan[0], fan_curve);
		if (CONFIG(BOARD_PROTECTLI_VP32XX))
			set_fan_curve(&ite_cfg->ec.fan[1], fan_curve);
	}
}

struct chip_operations mainboard_ops = {
	.final = mainboard_final,
	.enable_dev = mainboard_enable
};
