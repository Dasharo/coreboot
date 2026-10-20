/* SPDX-License-Identifier: GPL-2.0-only */

#include <console/console.h>
#include <device/pci_def.h>
#include <device/pci_ids.h>
#include <device/pci_ops.h>
#include <intelblocks/vtd.h>
#include <soc/pci_devs.h>
#include <soc/util.h>

static bool is_vtd_device(pci_devfn_t dev)
{
	const uint16_t did = pci_s_read_config16(dev, PCI_DEVICE_ID);

	if (pci_s_read_config16(dev, PCI_VENDOR_ID) != PCI_VID_INTEL)
		return false;

	/*
	 * Check the IDs separately, they are the same on some Xeon-SP generations
	 * and comparing them in one expression triggers -Wlogical-op.
	 */
	if (did == MMAP_VTD_CFG_REG_DEVID)
		return true;

	return did == MMAP_VTD_STACK_CFG_REG_DEVID;
}

/*
 * Xeon-SP has a VT-d engine in each IIO stack. Walk all stacks reported by
 * FSP and call the callback for each VT-d engine with an enabled VTD_BAR.
 * Only available after FSP-M, when the stack buses and VTD_BARs are assigned.
 */
void soc_vtd_for_each_pmr_engine(void (*func)(uintptr_t vtd_base))
{
	const IIO_UDS *hob = get_iio_uds();
	pci_devfn_t dev;
	uint32_t version;

	if (!hob)
		return;

	for (int s = 0; s < CONFIG_MAX_SOCKET; ++s) {
		if (!soc_cpu_is_enabled(s))
			continue;

		const unsigned int seg = hob->PlatformData.CpuQpiInfo[s].PcieSegment;

		if (seg >= PCI_SEGMENT_GROUP_COUNT) {
			printk(BIOS_WARNING, "VT-d: socket %d segment %u not accessible\n",
			       s, seg);
			continue;
		}

		for (int x = 0; x < MAX_LOGIC_IIO_STACK; ++x) {
			const xSTACK_RES *ri = &hob->PlatformData.IIO_resource[s].StackRes[x];

			if (ri->BusBase > ri->BusLimit)
				continue;

			dev = PCI_DEV(seg << 8 | ri->BusBase, VTD_DEV_NUM, VTD_FUNC_NUM);

			if (!is_vtd_device(dev))
				continue;

			const uint64_t vtd_bar = pci_s_read_config32(dev, VTD_BAR_CSR) |
				((uint64_t)pci_s_read_config32(dev, VTD_BAR_CSR + 4) << 32);

			/* VTD_BAR not enabled */
			if (!(vtd_bar & 1))
				continue;

			if (ENV_X86_32 && ((vtd_bar & ~0xfffULL) >= (4ULL * GiB))) {
				printk(BIOS_WARNING, "VT-d: socket %d stack %d bus 0x%02x "
				       "VTD_BAR 0x%08llx above 4G not accessible in 32-bit mode\n",
				       s, x, ri->BusBase, vtd_bar & ~0xfff);
				continue;
			}

			version = vtd_read32(vtd_bar & ~0xfff, VER_REG);

			/* VT-d registers not accessible through VTD_BAR */
			if (version == 0 || version == UINT32_MAX) {
				printk(BIOS_WARNING, "VT-d: socket %d stack %d bus 0x%02x "
				       "VTD_BAR 0x%08llx not accessible (VER_REG 0x%08x)\n",
				       s, x, ri->BusBase, vtd_bar & ~0xfff, version);
				continue;
			}

			printk(BIOS_DEBUG, "VT-d: socket %d stack %d bus 0x%02x "
			       "VTD_BAR 0x%08llx version %u.%u\n", s, x, ri->BusBase,
			       vtd_bar & ~0xfff, (version >> 4) & 0xf, version & 0xf);

			func(vtd_bar & ~0xfff);
		}
	}
}
