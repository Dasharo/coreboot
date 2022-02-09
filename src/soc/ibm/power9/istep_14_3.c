/* SPDX-License-Identifier: GPL-2.0-only */

#include <cpu/power/istep_14.h>

#include <console/console.h>
#include <cpu/power/proc.h>
#include <cpu/power/scom.h>

#include "pci.h"
#include "scratch.h"

static uint64_t pec_addr(uint8_t pec, uint64_t addr)
{
	return addr + pec*0x400;
}

static void init_pecs(uint8_t chip, const uint8_t *iovalid_enable)
{
	enum {
		P9N2_PEC_ADDREXTMASK_REG = 0x4010C05,
		PEC_PBCQHWCFG_REG = 0x4010C00,
		PEC_NESTTRC_REG = 0x4010C03,
		PEC_PBAIBHWCFG_REG = 0xD010800,

		PEC_PBCQHWCFG_REG_PE_DISABLE_TCE_ARBITRATION = 60,
		PEC_PBAIBHWCFG_REG_PE_PCIE_CLK_TRACE_EN = 30,
		PEC_AIB_HWCFG_OSBM_HOL_BLK_CNT = 40,
		PEC_AIB_HWCFG_OSBM_HOL_BLK_CNT_LEN = 3,
		PEC_PBCQHWCFG_REG_PE_DISABLE_OOO_MODE = 0x16,
		PEC_PBCQHWCFG_REG_PE_DISABLE_WR_SCOPE_GROUP = 42,
		PEC_PBCQHWCFG_REG_PE_CHANNEL_STREAMING_EN = 33,
		PEC_PBCQHWCFG_REG_PE_DISABLE_WR_VG = 41,
		PEC_PBCQHWCFG_REG_PE_DISABLE_INTWR_VG = 43,
		PEC_PBCQHWCFG_REG_PE_DISABLE_INTWR_SCOPE_GROUP = 44,
		PEC_PBCQHWCFG_REG_PE_DISABLE_RD_SCOPE_GROUP = 51,
		PEC_PBCQHWCFG_REG_PE_DISABLE_RD_VG = 54,
		PEC_PBCQHWCFG_REG_PE_DISABLE_TCE_SCOPE_GROUP = 56,
		PEC_PBCQHWCFG_REG_PE_DISABLE_TCE_VG = 59,
	};

	uint64_t scratch_reg6 = 0;
	uint8_t pec = 0;
	bool node_pump_mode = false;
	uint8_t dd = get_dd();

	scratch_reg6 = read_rscom(chip, MBOX_SCRATCH_REG1 + 5);

	/* ATTR_PROC_FABRIC_PUMP_MODE, it's either node or group pump mode */
	node_pump_mode = !(scratch_reg6 & PPC_BIT(MBOX_SCRATCH_REG6_GROUP_PUMP_MODE));

	for (pec = 0; pec < MAX_PEC_PER_PROC; ++pec) {
		uint64_t val = 0;

		printk(BIOS_EMERG, "Initializing PEC%d...\n", pec);

		/*
		 * ATTR_FABRIC_ADDR_EXTENSION_GROUP_ID = 0
		 * ATTR_FABRIC_ADDR_EXTENSION_CHIP_ID = 0
		 */
		rscom_and_or_for_chiplet(chip, N2_CHIPLET_ID,
					 pec_addr(pec, P9N2_PEC_ADDREXTMASK_REG),
					 ~PPC_BITMASK(0, 6),
					 PPC_PLACE(0, 0, 7));

		/*
		 * Phase2 init step 1
		 * NestBase + 0x00
		 * Set bits 00:03 = 0b0001 Set hang poll scale
		 * Set bits 04:07 = 0b0001 Set data scale
		 * Set bits 08:11 = 0b0001 Set hang pe scale
		 * Set bit 22 = 0b1 Disable out of order store behavior
		 * Set bit 33 = 0b1 Enable Channel Tag streaming behavior
		 * Set bits 34:35 = 0b11 Set P9 Style cache-inject behavior
		 * Set bits 46:48 = 0b011 Set P9 Style cache-inject rate, 1/16 cycles
		 * Set bit 60 = 0b1 only if PEC is bifurcated or trifurcated.
		 * if HW423589_option1, set Disable Group Scope (r/w) and Use Vg(sys) at Vg scope
		 */

		val = read_rscom_for_chiplet(chip, N2_CHIPLET_ID,
					     pec_addr(pec, PEC_PBCQHWCFG_REG));
		/* Set hang poll scale */
		val &= ~PPC_BITMASK(0, 3);
		val |= PPC_PLACE(1, 0, 4);
		/* Set data scale */
		val &= ~PPC_BITMASK(4, 7);
		val |= PPC_PLACE(1, 4, 4);
		/* Set hang pe scale */
		val &= ~PPC_BITMASK(8, 11);
		val |= PPC_PLACE(1, 8, 4);
		/* Disable out of order store behavior */
		val |= PPC_BIT(22);
		/* Enable Channel Tag streaming behavior */
		val |= PPC_BIT(33);

		/* Set Disable Group Scope (r/w) and Use Vg(sys) at Vg scope */
		val |= PPC_BIT(PEC_PBCQHWCFG_REG_PE_DISABLE_WR_VG);
		val |= PPC_BIT(PEC_PBCQHWCFG_REG_PE_DISABLE_WR_SCOPE_GROUP);
		val |= PPC_BIT(PEC_PBCQHWCFG_REG_PE_DISABLE_INTWR_VG);
		val |= PPC_BIT(PEC_PBCQHWCFG_REG_PE_DISABLE_INTWR_SCOPE_GROUP);
		val |= PPC_BIT(PEC_PBCQHWCFG_REG_PE_DISABLE_RD_VG);
		val |= PPC_BIT(PEC_PBCQHWCFG_REG_PE_DISABLE_RD_SCOPE_GROUP);
		val |= PPC_BIT(PEC_PBCQHWCFG_REG_PE_DISABLE_TCE_SCOPE_GROUP);
		val |= PPC_BIT(PEC_PBCQHWCFG_REG_PE_DISABLE_TCE_VG);

		/* Disable P9 Style cache injects if chip is node */
		if (!node_pump_mode) {
			/*
			 * ATTR_PROC_PCIE_CACHE_INJ_MODE
			 * Attribute to control the cache inject mode.
			 *
			 * DISABLE_CI      = 0x0 - Disable cache inject completely. (Reset value default)
			 * P7_STYLE_CI     = 0x1 - Use cache inject design from Power7.
			 * PCITLP_STYLE_CI = 0x2 - Use PCI TLP Hint bits in packet to perform the cache inject.
			 * P9_STYLE_CI     = 0x3 - Initial attempt as cache inject. Power9 style. (Attribute default)
			 *
			 * Different cache inject modes will affect DMA write performance. The attribute default was
			 * selected based on various workloads and was to be the most optimal settings for Power9.
			 * fapi2::ATTR_PROC_PCIE_CACHE_INJ_MODE = 3 by default
			 */
			val &= ~PPC_BITMASK(34, 35);
			val |= PPC_PLACE(0x3, 34, 2);

			if (dd == 0x21 || dd == 0x22 || dd == 0x23) {
				/*
				 * ATTR_PROC_PCIE_CACHE_INJ_THROTTLE
				 * Attribute to control the cache inject throttling when cache inject is enable.
				 *
				 * DISABLE   = 0x0 - Disable cache inject throttling. (Reset value default)
				 * 16_CYCLES = 0x1 - Perform 1 cache inject every 16 clock cycles.
				 * 32_CYCLES = 0x3 - Perform 1 cache inject every 32 clock cycles. (Attribute default)
				 * 64_CYCLES = 0x7 - Perform 1 cache inject every 32 clock cycles.
				 *
				 * Different throttle rates will affect DMA write performance. The attribute default
				 * settings were optimal settings found across various workloads.
				 */
				val &= ~PPC_BITMASK(46, 48);
				val |= PPC_PLACE(0x3, 46, 3);
			}
		}

		if (pec == 1 || (pec == 2 && iovalid_enable[pec] != 0x4))
			val |= PPC_BIT(PEC_PBCQHWCFG_REG_PE_DISABLE_TCE_ARBITRATION);

		write_rscom_for_chiplet(chip, N2_CHIPLET_ID, pec_addr(pec, PEC_PBCQHWCFG_REG),
					val);

		/*
		 * Phase2 init step 2
		 * NestBase + 0x01
		 * N/A Modify Drop Priority Control Register (DrPriCtl)
		 */

		/*
		 * Phase2 init step 3
		 * NestBase + 0x03
		 * Set bits 00:03 = 0b1001 Enable trace, and select
		 *                         inbound operations with addr information
		 */
		rscom_and_or_for_chiplet(chip, N2_CHIPLET_ID, pec_addr(pec, PEC_NESTTRC_REG),
					 ~PPC_BITMASK(0, 3),
					 PPC_PLACE(9, 0, 4));

		/*
		 * Phase2 init step 4
		 * NestBase + 0x05
		 * N/A For use of atomics/asb_notify
		 */

		/*
		 * Phase2 init step 5
		 * NestBase + 0x06
		 * N/A To override scope prediction
		 */

		/*
		 * Phase2 init step 6
		 * PCIBase +0x00
		 * Set bits 30 = 0b1 Enable Trace
		 */
		val = 0;
		val |= PPC_BIT(PEC_PBAIBHWCFG_REG_PE_PCIE_CLK_TRACE_EN);
		val |= PPC_PLACE(7, PEC_AIB_HWCFG_OSBM_HOL_BLK_CNT,
				 PEC_AIB_HWCFG_OSBM_HOL_BLK_CNT_LEN);
		write_rscom_for_chiplet(chip, PCI0_CHIPLET_ID + pec, PEC_PBAIBHWCFG_REG, val);
	}
}

/* See src/import/chips/p9/common/scominfo/p9_scominfo.C in Hostboot */
static void phb_write(uint8_t chip, uint8_t phb, uint64_t addr, uint64_t data)
{
	chiplet_id_t chiplet;
	uint8_t sat_id = (addr >> 6) & 0xF;

	if (phb == 0) {
		chiplet = PCI0_CHIPLET_ID;
		sat_id = (sat_id < 4 ? 1 : 4);
	} else {
		chiplet = PCI0_CHIPLET_ID + (phb / 3) + 1;
		sat_id = (sat_id < 4 ? 1 : 4)
		       + ((phb % 2) ? 0 : 1)
		       + (2 * (phb / 5));
	}

	addr &= ~PPC_BITMASK(54, 57);
	addr |= PPC_PLACE(sat_id, 54, 4);

	write_rscom_for_chiplet(chip, chiplet, addr, data);
}

/* See src/import/chips/p9/common/scominfo/p9_scominfo.C in Hostboot */
static void phb_nest_write(uint8_t chip, uint8_t phb, uint64_t addr, uint64_t data)
{
	enum { N2_PCIS0_0_RING_ID = 0x3 };

	uint8_t ring;
	uint8_t sat_id = (addr >> 6) & 0xF;

	if (phb == 0) {
		ring = (N2_PCIS0_0_RING_ID & 0xF);
		sat_id = (sat_id < 4 ? 1 : 4);
	} else {
		ring = ((N2_PCIS0_0_RING_ID + (phb / 3) + 1) & 0xF);
		sat_id = (sat_id < 4 ? 1 : 4)
		       + (phb % 2 ? 0 : 1)
		       + (2 * (phb / 5));
	}

	addr &= ~PPC_BITMASK(50, 53);
	addr |= PPC_PLACE(ring, 50, 4);

	addr &= ~PPC_BITMASK(54, 57);
	addr |= PPC_PLACE(sat_id, 54, 4);

	write_rscom_for_chiplet(chip, N2_CHIPLET_ID, addr, data);
}

static void init_phbs(uint8_t chip, uint8_t phb_active_mask, const uint8_t *iovalid_enable)
{
	enum {
		PHB_CERR_RPT0_REG = 0x4010C4A,
		PHB_CERR_RPT1_REG = 0x4010C4B,
		PHB_NFIR_REG = 0x4010C40,
		PHB_NFIRWOF_REG = 0x4010C48,

		PHB_NFIRACTION0_REG = 0x4010C46,
		PCI_NFIR_ACTION0_REG = 0x5B0F81E000000000,

		PHB_NFIRACTION1_REG = 0x4010C47,
		PCI_NFIR_ACTION1_REG = 0x7F0F81E000000000,

		PHB_NFIRMASK_REG = 0x4010C43,
		PCI_NFIR_MASK_REG = 0x30001C00000000,

		PHB_PE_DFREEZE_REG = 0x4010C55,
		PHB_PBAIB_CERR_RPT_REG = 0xD01084B,
		PHB_PFIR_REG = 0xD010840,
		PHB_PFIRWOF_REG = 0xD010848,

		PHB_PFIRACTION0_REG = 0xD010846,
		PCI_PFIR_ACTION0_REG = 0xB000000000000000,

		PHB_PFIRACTION1_REG = 0xD010847,
		PCI_PFIR_ACTION1_REG = 0xB000000000000000,

		PHB_PFIRMASK_REG = 0xD010843,
		PCI_PFIR_MASK_REG = 0xE00000000000000,

		P9_PCIE_CONFIG_BAR_SHIFT = 8,

		PHB_MMIOBAR0_REG = 0x4010C4E,
		PHB_MMIOBAR0_MASK_REG = 0x4010C4F,
		PHB_MMIOBAR1_REG = 0x4010C50,
		PHB_MMIOBAR1_MASK_REG = 0x04010C51,
		PHB_PHBBAR_REG = 0x4010C52,
		PHB_BARE_REG = 0x4010C54,

		PHB_PHBRESET_REG = 0xD01084A,
		PHB_ACT0_REG = 0xD01090E,
		PHB_ACTION1_REG = 0xD01090F,
		PHB_MASK_REG = 0xD01090B,
	};

	/* ATTR_PROC_PCIE_MMIO_BAR0_BASE_ADDR_OFFSET */
	const uint64_t mmio_bar0_offsets[MAX_PHB_PER_PROC] = { 0 };
	/* ATTR_PROC_PCIE_MMIO_BAR1_BASE_ADDR_OFFSET */
	const uint64_t mmio_bar1_offsets[MAX_PHB_PER_PROC] = { 0 };
	/* ATTR_PROC_PCIE_REGISTER_BAR_BASE_ADDR_OFFSET */
	const uint64_t register_bar_offsets[MAX_PHB_PER_PROC] = { 0 };
	/* ATTR_PROC_PCIE_BAR_SIZE */
	const uint64_t bar_sizes[3] = { 0 };

	/* Base address of chip MMIO range */
	const uint64_t base_addr_mmio = PROC_BASE_ADDR(chip, /*msel=*/0x3);

	uint8_t phb = 0;
	for (phb = 0; phb < MAX_PHB_PER_PROC; ++phb) {
		/* BAR enable attribute (ATTR_PROC_PCIE_BAR_ENABLE) */
		const uint8_t bar_enables[3] = { 0 };

		uint64_t val = 0;
		uint64_t mmio0_bar = base_addr_mmio;
		uint64_t mmio1_bar = base_addr_mmio;
		uint64_t register_bar = base_addr_mmio;

		if (!(phb_active_mask & (PHB0_MASK >> phb)))
			continue;

		printk(BIOS_EMERG, "Initializing PHB%d...\n", phb);

		/*
		 * Phase2 init step 12_a (yes, out of order)
		 * NestBase + StackBase + 0xA
		 * 0xFFFFFFFF_FFFFFFFF
		 * Clear any spurious cerr_rpt0 bits (cerr_rpt0)
		 */
		phb_nest_write(chip, phb, PHB_CERR_RPT0_REG, PPC_BITMASK(0, 63));

		/*
		 * Phase2 init step 12_b (yes, out of order)
		 * NestBase + StackBase + 0xB
		 * 0xFFFFFFFF_FFFFFFFF
		 * Clear any spurious cerr_rpt1 bits (cerr_rpt1)
		 */
		phb_nest_write(chip, phb, PHB_CERR_RPT1_REG, PPC_BITMASK(0, 63));

		/*
		 * Phase2 init step 7_c
		 * NestBase + StackBase + 0x0
		 * 0x00000000_00000000
		 * Clear any spurious FIR
		 * bits (NFIR)NFIR
		 */
		phb_nest_write(chip, phb, PHB_NFIR_REG, 0);

		/*
		 * Phase2 init step 8
		 * NestBase + StackBase + 0x8
		 * 0x00000000_00000000
		 * Clear any spurious WOF bits (NFIRWOF)
		 */
		phb_nest_write(chip, phb, PHB_NFIRWOF_REG, 0);

		/*
		 * Phase2 init step 9
		 * NestBase + StackBase + 0x6
		 * Set the per FIR Bit Action 0 register
		 */
		phb_nest_write(chip, phb, PHB_NFIRACTION0_REG, PCI_NFIR_ACTION0_REG);

		/*
		 * Phase2 init step 10
		 * NestBase + StackBase + 0x7
		 * Set the per FIR Bit Action 1 register
		 */
		phb_nest_write(chip, phb, PHB_NFIRACTION1_REG, PCI_NFIR_ACTION1_REG);

		/*
		 * Phase2 init step 11
		 * NestBase + StackBase + 0x3
		 * Set FIR Mask Bits to allow errors (NFIRMask)
		 */
		phb_nest_write(chip, phb, PHB_NFIRMASK_REG, PCI_NFIR_MASK_REG);

		/*
		 * Phase2 init step 12
		 * NestBase + StackBase + 0x15
		 * 0x00000000_00000000
		 * Set Data Freeze Type Register for SUE handling (DFREEZE)
		 */
		phb_nest_write(chip, phb, PHB_PE_DFREEZE_REG, 0);

		/*
		 * Phase2 init step 13_a
		 * PCIBase + StackBase + 0xB
		 * 0x00000000_00000000
		 * Clear any spurious pbaib_cerr_rpt bits
		 */
		phb_write(chip, phb, PHB_PBAIB_CERR_RPT_REG, 0);

		/*
		 * Phase2 init step 13_b
		 * PCIBase + StackBase + 0x0
		 * 0x00000000_00000000
		 * Clear any spurious FIR
		 * bits (PFIR)PFIR
		 */
		phb_write(chip, phb, PHB_PFIR_REG, 0);

		/*
		 * Phase2 init step 14
		 * PCIBase + StackBase + 0x8
		 * 0x00000000_00000000
		 * Clear any spurious WOF bits (PFIRWOF)
		 */
		phb_write(chip, phb, PHB_PFIRWOF_REG, 0);

		/*
		 * Phase2 init step 15
		 * PCIBase + StackBase + 0x6
		 * Set the per FIR Bit Action 0 register
		 */
		phb_write(chip, phb, PHB_PFIRACTION0_REG, PCI_PFIR_ACTION0_REG);

		/*
		 * Phase2 init step 16
		 * PCIBase + StackBase + 0x7
		 * Set the per FIR Bit Action 1 register
		 */
		phb_write(chip, phb, PHB_PFIRACTION1_REG, PCI_PFIR_ACTION1_REG);

		/*
		 * Phase2 init step 17
		 * PCIBase + StackBase + 0x3
		 * Set FIR Mask Bits to allow errors (PFIRMask)
		 */
		phb_write(chip, phb, PHB_PFIRMASK_REG, PCI_PFIR_MASK_REG);

		/*
		 * Phase2 init step 18
		 * NestBase + StackBase + 0xE
		 * Set MMIO Base Address Register 0 (MMIOBAR0)
		 */
		mmio0_bar += mmio_bar0_offsets[phb];
		mmio0_bar <<= P9_PCIE_CONFIG_BAR_SHIFT;
		phb_nest_write(chip, phb, PHB_MMIOBAR0_REG, mmio0_bar);

		/*
		 * Phase2 init step 19
		 * NestBase + StackBase + 0xF
		 * Set MMIO BASE Address Register Mask 0 (MMIOBAR0_MASK)
		 */
		phb_nest_write(chip, phb, PHB_MMIOBAR0_MASK_REG, bar_sizes[0]);

		/*
		 * Phase2 init step 20
		 * NestBase + StackBase + 0x10
		 * Set MMIO Base
		 * Address Register 1 (MMIOBAR1)
		 */
		mmio1_bar += mmio_bar1_offsets[phb];
		mmio1_bar <<= P9_PCIE_CONFIG_BAR_SHIFT;
		phb_nest_write(chip, phb, PHB_MMIOBAR1_REG, mmio1_bar);

		/*
		 * Phase2 init step 21
		 * NestBase + StackBase + 0x11
		 * Set MMIO Base Address Register Mask 1 (MMIOBAR1_MASK)
		 */
		phb_nest_write(chip, phb, PHB_MMIOBAR1_MASK_REG, bar_sizes[1]);

		/*
		 * Phase2 init step 22
		 * NestBase + StackBase + 0x12
		 * Set PHB Register Base address Register (PHBBAR)
		 */
		register_bar += register_bar_offsets[phb];
		register_bar <<= P9_PCIE_CONFIG_BAR_SHIFT;
		phb_nest_write(chip, phb, PHB_PHBBAR_REG, register_bar);

		/*
		 * Phase2 init step 23
		 * NestBase + StackBase + 0x14
		 * Set Base address Enable Register (BARE)
		 */

		val = 0;

		if (bar_enables[0])
			val |= PPC_BIT(0); // PHB_BARE_REG_PE_MMIO_BAR0_EN, bit 0 for BAR0
		if (bar_enables[1])
			val |= PPC_BIT(1); // PHB_BARE_REG_PE_MMIO_BAR1_EN, bit 1 for BAR1
		if (bar_enables[2])
			val |= PPC_BIT(2); // PHB_BARE_REG_PE_PHB_BAR_EN, bit 2 for PHB

		phb_nest_write(chip, phb, PHB_BARE_REG, val);

		/*
		 * Phase2 init step 24
		 * PCIBase + StackBase +0x0A
		 * 0x00000000_00000000
		 * Remove ETU/AIB bus from reset (PHBReset)
		 */
		phb_write(chip, phb, PHB_PHBRESET_REG, 0);
		/* Configure ETU FIR (all masked) */
		phb_write(chip, phb, PHB_ACT0_REG, 0);
		phb_write(chip, phb, PHB_ACTION1_REG, 0);
		phb_write(chip, phb, PHB_MASK_REG, PPC_BITMASK(0, 63));
	}
}

void istep_14_3(uint8_t chips, const struct pci_info *pci_info)
{
	printk(BIOS_EMERG, "starting istep 14.3\n");
	report_istep(14,3);

	for (uint8_t chip = 0; chip < MAX_CHIPS; chip++) {
		if (!(chips & (1 << chip)))
			continue;

		init_pecs(chip, pci_info[chip].iovalid_enable);
		init_phbs(chip, pci_info[chip].phb_active_mask, pci_info[chip].iovalid_enable);
	}

	printk(BIOS_EMERG, "ending istep 14.3\n");
}
