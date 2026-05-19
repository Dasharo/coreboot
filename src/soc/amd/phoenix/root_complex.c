/* SPDX-License-Identifier: GPL-2.0-only */

/* TODO: Update for Phoenix */

#include <acpi/acpi.h>
#include <acpi/acpigen.h>
#include <acpi/acpigen_pci.h>
#include <acpi/acpi_device.h>
#include <amdblocks/alib.h>
#include <amdblocks/amd_pci_util.h>
#include <amdblocks/data_fabric.h>
#include <amdblocks/ioapic.h>
#include <amdblocks/root_complex.h>
#include <arch/ioapic.h>
#include <device/device.h>
#include <device/pci.h>
#include <soc/iomap.h>
#include <static.h>
#include <stdint.h>
#include "chip.h"

#define DPTC_TOTAL_UPDATE_PARAMS	7

struct dptc_input {
	uint16_t size;
	struct alib_dptc_param params[DPTC_TOTAL_UPDATE_PARAMS];
} __packed;

#define DPTC_INPUTS(_thermctllmit, _sustained, _fast, _slow,			\
	_vrmCurrentLimit, _vrmMaxCurrentLimit, _vrmSocCurrentLimit)		\
	{									\
		.size = sizeof(struct dptc_input),				\
		.params = {							\
			{							\
				.id = ALIB_DPTC_THERMAL_CONTROL_LIMIT_ID,	\
				.value = _thermctllmit,				\
			},							\
			{							\
				.id = ALIB_DPTC_SUSTAINED_POWER_LIMIT_ID,	\
				.value = _sustained,				\
			},							\
			{							\
				.id = ALIB_DPTC_FAST_PPT_LIMIT_ID,		\
				.value = _fast,					\
			},							\
			{							\
				.id = ALIB_DPTC_SLOW_PPT_LIMIT_ID,		\
				.value = _slow,					\
			},							\
			{							\
				.id = ALIB_DPTC_VRM_CURRENT_LIMIT_ID,		\
				.value = _vrmCurrentLimit,			\
			},							\
			{							\
				.id = ALIB_DPTC_VRM_MAXIMUM_CURRENT_LIMIT,	\
				.value = _vrmMaxCurrentLimit,			\
			},							\
			{							\
				.id = ALIB_DPTC_VRM_SOC_CURRENT_LIMIT_ID,	\
				.value = _vrmSocCurrentLimit,			\
			},							\
		},								\
	}

static void acipgen_dptci(void)
{
	const struct soc_amd_phoenix_config *config = config_of_soc();

	/* Normal mode DPTC values. */
	struct dptc_input default_input = DPTC_INPUTS(config->thermctl_limit_degreeC,
		config->sustained_power_limit_mW,
		config->fast_ppt_limit_mW,
		config->slow_ppt_limit_mW,
		config->vrm_current_limit_mA,
		config->vrm_maximum_current_limit_mA,
		config->vrm_soc_current_limit_mA);
	acpigen_write_alib_dptc_default((uint8_t *)&default_input, sizeof(default_input));

	/* Low/No Battery */
	struct dptc_input no_battery_input = DPTC_INPUTS(
		config->thermctl_limit_degreeC,
		config->sustained_power_limit_mW,
		config->fast_ppt_limit_mW,
		config->slow_ppt_limit_mW,
		config->vrm_current_limit_throttle_mA,
		config->vrm_maximum_current_limit_throttle_mA,
		config->vrm_soc_current_limit_throttle_mA);
	acpigen_write_alib_dptc_no_battery((uint8_t *)&no_battery_input,
		sizeof(no_battery_input));
}

struct pci_dev_int_routes {
	unsigned int devfn;
	unsigned int num_irqs;
	unsigned int irq_base;
};

static const struct pci_dev_int_routes iohc_devs[] = {
	{ .devfn = PCI_DEVFN(0x14, 0), .num_irqs = 4, .irq_base = 16 },
	{ .devfn = PCI_DEVFN(0x08, 0), .num_irqs = 1, .irq_base = 28 },
};

static void acpigen_write_PRT_GSI(const struct device *rb)
{
	char *pkg_count;
	const struct device *dev;

	pkg_count = acpigen_write_package(0); /* Package - APIC Routing */

	for (unsigned int d = 0; d < ARRAY_SIZE(iohc_devs); d++) {
		dev = pcidev_path_behind(rb->upstream, iohc_devs[d].devfn);
		if (!dev || !dev->enabled)
			continue;

		for (unsigned int i = 0; i < iohc_devs[d].num_irqs; ++i) {
			(*pkg_count)++;
			acpigen_write_PRT_GSI_entry(
				PCI_SLOT(iohc_devs[d].devfn),
				i, /* pin */
				iohc_devs[d].irq_base + i);
		}
	}

	acpigen_pop_len(); /* Package - APIC Routing */
}

static void acpigen_write_PRT_PIC(const struct device *rb)
{
	char link_template[] = "\\_SB.INTX";
	char *pkg_count;
	const struct device *dev;

	pkg_count = acpigen_write_package(0); /* Package - PIC Routing */
	for (unsigned int d = 0; d < ARRAY_SIZE(iohc_devs); d++) {
		dev = pcidev_path_behind(rb->upstream, iohc_devs[d].devfn);
		if (!dev || !dev->enabled)
			continue;

		for (unsigned int i = 0; i < iohc_devs[d].num_irqs; i++) {
			link_template[8] = 'A' + ((iohc_devs[d].irq_base + i) % 8);
			(*pkg_count)++;
			acpigen_write_PRT_source_entry(
				PCI_SLOT(iohc_devs[d].devfn),
				i, /* pin */
				link_template /* Source */,
				0 /* Source Index */);
		}
	}

	acpigen_pop_len(); /* Package - PIC Routing */
}

static void acpigen_write_host_bridge_PRT(const struct device *dev)
{
	acpigen_write_method("_PRT", 0);

	/* If (PICM) */
	acpigen_write_if();
	acpigen_emit_namestring("PICM");

	/* Return (Package{...}) */
	acpigen_emit_byte(RETURN_OP);
	acpigen_write_PRT_GSI(dev);

	/* Else */
	acpigen_write_else();

	/* Return (Package{...}) */
	acpigen_emit_byte(RETURN_OP);
	acpigen_write_PRT_PIC(dev);

	acpigen_pop_len(); /* End Else */

	acpigen_pop_len(); /* Method */
}

static void root_complex_fill_ssdt(const struct device *device)
{
	const char *acpi_scope = acpi_device_path(dev_get_domain(device));

	if (CONFIG(SOC_AMD_COMMON_BLOCK_ACPI_DPTC))
		acipgen_dptci();

	acpigen_write_scope(acpi_scope);

	printk(BIOS_DEBUG, "%s: writing _PRT\n", acpi_scope);
	acpigen_write_host_bridge_PRT(device);

	acpigen_pop_len(); /* Scope */
}

static const char *gnb_acpi_name(const struct device *dev)
{
	return "GNB";
}

struct device_operations phoenix_root_complex_operations = {
	/* The root complex has no PCI BARs implemented, so there's no need to call
	   pci_dev_read_resources for it */
	.read_resources		= noop_read_resources,
	.set_resources		= noop_set_resources,
	.enable_resources	= pci_dev_enable_resources,
	.acpi_name		= gnb_acpi_name,
	.acpi_fill_ssdt		= root_complex_fill_ssdt,
};

static const struct domain_iohc_info iohc_info[] = {
	[0] = {
		.fabric_id = IOMS0_FABRIC_ID,
		.misc_smn_base = SMN_IOHC_MISC_BASE_13B1,
	},
};

const struct domain_iohc_info *get_iohc_info(size_t *count)
{
	*count = ARRAY_SIZE(iohc_info);
	return iohc_info;
}

static const struct non_pci_mmio_reg non_pci_mmio[] = {
	{ 0x2d0, 0xfffffff00000ull,   1 * MiB, NON_PCI_RES_IDX_AUTO },
	{ 0x2d8, 0xfffffff00000ull,   1 * MiB, NON_PCI_RES_IDX_AUTO },
	{ 0x2e0, 0xfffffff00000ull,   1 * MiB, NON_PCI_RES_IDX_AUTO },
	{ 0x2e8, 0xfffffff00000ull,   1 * MiB, NON_PCI_RES_IDX_AUTO },
	/* The hardware has a 256 byte alignment requirement for the IOAPIC MMIO base, but we
	   tell the FSP to configure a 4k-aligned base address and this is reported as 4 KiB
	   resource. */
	{ 0x2f0, 0xffffffffff00ull,   4 * KiB, IOMMU_IOAPIC_IDX },
	{ 0x2f8, 0xfffffff00000ull,   1 * MiB, NON_PCI_RES_IDX_AUTO },
	{ 0x300, 0xfffffff00000ull,   1 * MiB, NON_PCI_RES_IDX_AUTO },
	{ 0x308, 0xfffffffff000ull,   4 * KiB, NON_PCI_RES_IDX_AUTO },
	{ 0x310, 0xfffffff00000ull,   1 * MiB, NON_PCI_RES_IDX_AUTO },
	{ 0x318, 0xfffffff80000ull, 512 * KiB, NON_PCI_RES_IDX_AUTO },
	{ 0x320, 0xfffffff00000ull,   1 * MiB, NON_PCI_RES_IDX_AUTO },
};

const struct non_pci_mmio_reg *get_iohc_non_pci_mmio_regs(size_t *count)
{
	*count = ARRAY_SIZE(non_pci_mmio);
	return non_pci_mmio;
}
