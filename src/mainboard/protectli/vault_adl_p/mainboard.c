/* SPDX-License-Identifier: GPL-2.0-only */

#include <arch/cpu.h>
#include <arch/io.h>
#include <delay.h>
#include <device/device.h>
#include <smbios.h>
#include <soc/ramstage.h>

#include <string.h>

#define ITE_GPIO_BASE		0xa00
#define ITE_GPIO_PIN(x)		(1 << ((x) % 10))
#define ITE_GPIO_SET(x)		(((x) / 10) - 1)
#define ITE_GPIO_IO_ADDR(x)	(ITE_GPIO_BASE + ITE_GPIO_SET(x))

static void do_beep(uint32_t frequency, uint32_t duration_msec)
{
	uint32_t timer_delay = 1000000 / frequency / 2;
	uint32_t count = (duration_msec * 1000) / (timer_delay * 2);
	uint8_t val = inb(ITE_GPIO_IO_ADDR(41)); /* GP41 drives a MOSFET for PC Speaker */

	for (uint32_t i = 0; i < count; i++)
	{
		outb(val | ITE_GPIO_PIN(41), ITE_GPIO_IO_ADDR(41));
		udelay(timer_delay);
		outb(val & ~ITE_GPIO_PIN(41), ITE_GPIO_IO_ADDR(41));
		udelay(timer_delay);
	}
}

const char *smbios_mainboard_product_name(void)
{
	u32 tmp[13];
	const char *str = "Unknown Processor Name";

	if (cpu_have_cpuid()) {
		int i;
		struct cpuid_result res = cpuid(0x80000000);
		if (res.eax >= 0x80000004) {
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

	if (strstr(str, "i3-1215U") != NULL)
		return "VP6630";
	else if (strstr(str, "i5-1235U") != NULL)
		return "VP6650";
	else if (strstr(str, "i7-1255U") != NULL)
		return "VP6670";
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

	// Type-C PD
	params->PmcPdEnable = 1;
	params->PchSerialIoI2cSdaPinMux[1] = 0x1947c606; // GPP_H6
	params->PchSerialIoI2cSclPinMux[1] = 0x1947a607; // GPP_H7
	params->PortResetMessageEnable[7] = 1;

	// IOM USB config
	params->PchUsbOverCurrentEnable = 0;

	params->EnableTcssCovTypeA[0] = 1;
	params->EnableTcssCovTypeA[1] = 1;
	params->EnableTcssCovTypeA[3] = 1;

	params->MappingPchXhciUsbA[0] = 1;
	params->MappingPchXhciUsbA[1] = 2;
	params->MappingPchXhciUsbA[3] = 4;

	params->CnviRfResetPinMux = 0;
	params->CnviClkreqPinMux = 0;
}

static void mainboard_final(void *chip_info)
{
	if (CONFIG(BEEP_ON_BOOT))
		do_beep(1500, 100);
}

struct chip_operations mainboard_ops = {
	.final = mainboard_final,
};
