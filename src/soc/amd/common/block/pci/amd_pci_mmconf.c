/* SPDX-License-Identifier: GPL-2.0-only */

#define __SIMPLE_DEVICE__

#include <amdblocks/amd_pci_mmconf.h>
#include <amdblocks/smn.h>
#include <commonlib/bsd/compiler.h>
#include <cpu/amd/msr.h>
#include <cpu/x86/msr.h>
#include <cpu/x86/mtrr.h>
#include <lib.h>

#define SMN_D18F0_BASE			0x49000000
#define D18F0_PCI_MMCONF_BASE_LO	0xc10
#define D18F0_PCI_MMCONF_BASE_HI	0xc14
#define D18F0_PCI_MMCONF_LIMIT_LO	0xc18
#define D18F0_PCI_MMCONF_LIMIT_HI	0xc1c

__weak uint32_t soc_get_df_func0_smn_base(void)
{
	return SMN_D18F0_BASE;
}

static bool df_pci_mmconf_needs_update(uint64_t mmconf_base, uint64_t mmconf_limit)
{
	uint8_t needs_update = 0;
	uint32_t smn_base = soc_get_df_func0_smn_base();
	uint32_t reg;

	reg = smn_io_read32(smn_base + D18F0_PCI_MMCONF_BASE_LO);
	needs_update |= (reg != (mmconf_base & 0xffffffff));

	reg = smn_io_read32(smn_base + D18F0_PCI_MMCONF_BASE_HI);
	needs_update |= (reg != (mmconf_base >> 32));

	reg = smn_io_read32(smn_base + D18F0_PCI_MMCONF_LIMIT_LO);
	needs_update |= (reg != (mmconf_limit & 0xffffffff));

	reg = smn_io_read32(smn_base + D18F0_PCI_MMCONF_LIMIT_HI);
	needs_update |= (reg != (mmconf_limit >> 32));

	return !!needs_update;
}

static void df_set_pci_mmconf(void)
{
	uint32_t reg;
	uint32_t smn_base = soc_get_df_func0_smn_base();
	uint64_t mmconf_base = CONFIG_ECAM_MMCONF_BASE_ADDRESS;
	uint64_t mmconf_limit = mmconf_base + CONFIG_ECAM_MMCONF_LENGTH;

	mmconf_limit--;
	mmconf_limit &= 0xfff00000; /* Address bits [19:0] are fixed to be FFFFF */
	mmconf_base |= 1; /* Range enable */

	if (!df_pci_mmconf_needs_update(mmconf_base, mmconf_limit))
		return;

	/*
	 * We have to use I/O PCI access to SMN index/data, because MMCONF
	 * will not work with our MMCONF address until this function returns.
	 */
	reg = smn_io_read32(smn_base + D18F0_PCI_MMCONF_BASE_LO);
	reg &= ~1; /* Disable MMCONF range first */
	smn_io_write32(smn_base + D18F0_PCI_MMCONF_BASE_LO, reg);

	/* Now repeat the order in which ABL configured the MMCONF */
	reg = mmconf_limit >> 32;
	smn_io_write32(smn_base + D18F0_PCI_MMCONF_LIMIT_HI, reg);
	reg = mmconf_limit & 0xffffffff;
	smn_io_write32(smn_base + D18F0_PCI_MMCONF_LIMIT_LO, reg);
	reg = mmconf_base >> 32;
	smn_io_write32(smn_base + D18F0_PCI_MMCONF_BASE_HI, reg);
	reg = mmconf_base & 0xffffffff;
	smn_io_write32(smn_base + D18F0_PCI_MMCONF_BASE_LO, reg);
}

void enable_pci_mmconf(void)
{
	msr_t mmconf;

	if (CONFIG(SOC_AMD_COMMON_BLOCK_PCI_MMCONF_SYNC_WITH_DF))
		df_set_pci_mmconf();

	mmconf.hi = (uint64_t)CONFIG_ECAM_MMCONF_BASE_ADDRESS >> 32;
	mmconf.lo = (CONFIG_ECAM_MMCONF_BASE_ADDRESS & 0xfff00000) | MMIO_RANGE_EN
			| __fls(CONFIG_ECAM_MMCONF_BUS_NUMBER) << MMIO_BUS_RANGE_SHIFT;
	wrmsr(MMIO_CONF_BASE, mmconf);
}
