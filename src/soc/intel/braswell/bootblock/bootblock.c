/* SPDX-License-Identifier: GPL-2.0-only */

#include <arch/io.h>
#include <bootblock_common.h>
#include <cf9_reset.h>
#include <console/console.h>
#include <cpu/x86/bist.h>
#include <cpu/x86/msr.h>
#include <device/mmio.h>
#include <device/pci_def.h>
#include <device/pci_ops.h>
#include <fsp/util.h>
#include <gpio.h>
#include <pc80/mc146818rtc.h>
#include <soc/iomap.h>
#include <soc/iosf.h>
#include <soc/lpc.h>
#include <soc/msr.h>
#include <soc/pm.h>
#include <soc/spi.h>
#include <version.h>

#include "../chip.h"

#define DEVEN			0x54	/* Device Enable */

#define PUNIT_BIOS_RESET_CPL	0x5
#define   BIOS_RESET_DONE	(1 << 0)
#define   BIOS_ALL_DONE		(1 << 1)
#define   PUNIT_RTOS_DONE	(1 << 8)

static uint32_t saved_bist;

asmlinkage void bootblock_c_entry_bist(uint64_t base_timestamp, uint32_t bist)
{
	saved_bist = bist;
	/* Call lib/bootblock.c main */
	bootblock_main_with_basetime(base_timestamp);
}

asmlinkage void bootblock_c_entry(uint64_t base_timestamp)
{
	/* Call lib/bootblock.c main */
	bootblock_main_with_basetime(base_timestamp);
}

static void program_base_addresses(void)
{
	uint32_t reg;
	const uint32_t lpc_dev = PCI_DEV(0, LPC_DEV, LPC_FUNC);
	const uint32_t smbus_dev = PCI_DEV(0, SMBUS_DEV, SMBUS_FUNC);

	/* Memory Mapped IO registers. */
	reg = PMC_BASE_ADDRESS | 2;
	pci_write_config32(lpc_dev, PBASE, reg);
	reg = IO_BASE_ADDRESS | 2;
	pci_write_config32(lpc_dev, IOBASE, reg);
	reg = ILB_BASE_ADDRESS | 2;
	pci_write_config32(lpc_dev, IBASE, reg);
	reg = SPI_BASE_ADDRESS | 2;
	pci_write_config32(lpc_dev, SBASE, reg);
	reg = MPHY_BASE_ADDRESS | 2;
	pci_write_config32(lpc_dev, MPBASE, reg);
	reg = PUNIT_BASE_ADDRESS | 2;
	pci_write_config32(lpc_dev, PUBASE, reg);
	reg = RCBA_BASE_ADDRESS | 1;
	pci_write_config32(lpc_dev, RCBA, reg);

	reg = SMBUS_MEMBASE_ADDRESS;
	pci_write_config32(smbus_dev, PCI_BASE_ADDRESS_0, reg);
	reg = 0;
	pci_write_config32(smbus_dev, PCI_BASE_ADDRESS_1, reg);

	/* IO Port Registers. */
	reg = ACPI_BASE_ADDRESS | 2;
	pci_write_config32(lpc_dev, ABASE, reg);
	reg = GPIO_BASE_ADDRESS | 2;
	pci_write_config32(lpc_dev, GBASE, reg);
	reg = SMBUS_BASE_ADDRESS;
	pci_write_config32(smbus_dev, PCI_BASE_ADDRESS_4, reg);
}

static void tco_disable(void)
{
	uint32_t reg;

	reg = inl(ACPI_BASE_ADDRESS + TCO1_CNT);
	reg |= TCO_TMR_HALT;
	outl(reg, ACPI_BASE_ADDRESS + TCO1_CNT);
}

static void spi_init(void)
{
	void *scs = (void *)(SPI_BASE_ADDRESS + SCS);
	void *bcr = (void *)(SPI_BASE_ADDRESS + BCR);
	uint32_t reg;

	/* Disable generating SMI when setting WPD bit. */
	write32(scs, read32(scs) & ~SMIWPEN);
	/*
	 * Enable caching and prefetching in the SPI controller. Disable
	 * the SMM-only BIOS write and set WPD bit.
	 */
	reg = (read32(bcr) & ~SRC_MASK) | SRC_CACHE_PREFETCH | BCR_WPD;
	reg &= ~EISS;
	write32(bcr, reg);
}

static void soc_rtc_init(void)
{
	int rtc_failed = rtc_failure();

	if (rtc_failed)
		printk(BIOS_ERR, "RTC Failure detected. Resetting date to %x/%x/%x%x\n",
			coreboot_build_date.month, coreboot_build_date.day,
			coreboot_build_date.century, coreboot_build_date.year);

	cmos_init(rtc_failed);
}

static void setup_mmconfig(void)
{
	uint32_t reg;

	/*
	 * Set up the MMCONF range. The register lives in the BUNIT. The IO variant of the
	 * config access needs to be used initially to properly configure as the IOSF access
	 * registers live in PCI config space.
	 */
	reg = 0;
	/* Clear the extended register. */
	pci_io_write_config32(IOSF_PCI_DEV, MCRX_REG, reg);
	reg = CONFIG_ECAM_MMCONF_BASE_ADDRESS | 1;
	pci_io_write_config32(IOSF_PCI_DEV, MDR_REG, reg);
	reg = IOSF_OPCODE(IOSF_OP_WRITE_BUNIT) | IOSF_PORT(IOSF_PORT_BUNIT) |
	      IOSF_REG(BUNIT_MMCONF_REG) | IOSF_BYTE_EN;
	pci_io_write_config32(IOSF_PCI_DEV, MCR_REG, reg);
}

static void enable_devices(void)
{
	pci_io_write_config32(PCI_DEV(0, SOC_DEV, SOC_FUNC), DEVEN, 0xb8000019);
}

static void set_max_non_turbo_freq(void)
{
	msr_t perf_ctl;
	msr_t msr;

	const struct cpuid_result cpuid1 = cpuid(1);

	/* Is enhanced speedstep supported? */
	if (!(cpuid1.ecx & (1 << 7)) || (rdmsr(IA32_PLATFORM_ID).lo & (1 << 17)))
		return;

	/* Enable Intel SpeedStep */
	msr = rdmsr(IA32_MISC_ENABLE);
	msr.lo |= (1 << 16);
	wrmsr(IA32_MISC_ENABLE, msr);

	/* Enable Burst Mode */
	msr = rdmsr(IA32_MISC_ENABLE);
	msr.hi = 0;
	wrmsr(IA32_MISC_ENABLE, msr);

	/* Set guaranteed ratio [21:16] from IACORE_RATIOS to bits [15:8] of the PERF_CTL */
	msr = rdmsr(MSR_IACORE_RATIOS);
	perf_ctl.lo = (msr.lo & 0x003f0000) >> 8;

	/* Set guaranteed vid [21:16] from IACORE_VIDS to bits [7:0] of the PERF_CTL */
	msr = rdmsr(MSR_IACORE_VIDS);
	perf_ctl.lo |= (msr.lo & 0x007f0000) >> 16;
	perf_ctl.hi = 0;

	wrmsr(IA32_PERF_CTL, perf_ctl);
}

static void init_hpet_registers(void)
{
	write32((void *)HPET_BASE_ADDRESS + 0x108, 0);
	write32((void *)HPET_BASE_ADDRESS + 0x10C, 0);
	/* Ensure FSP_GLOBAL_DATA pointer is invalid for FSP memory init */
	write32((void *)HPET_BASE_ADDRESS + 0x148, 0xffffffff);
}

static void usb_workaround_for_port(unsigned int port)
{
	u32 reg;

	/* Also described in DOC#556192 */
	iosf_usbphy_write(0x4013 + port * 0x100, 0x00000008);
	iosf_usbphy_write(0x4023 + port * 0x100, 0x00007F13);
	iosf_usbphy_write(0x4023 + port * 0x100, 0x00000013);
	iosf_usbphy_write(0x4023 + port * 0x100, 0x00004011);
	iosf_usbphy_write(0x4013 + port * 0x100, 0x00000108);
	iosf_usbphy_write(0x4013 + port * 0x100, 0x00000008);
	iosf_usbphy_write(0x4013 + port * 0x100, 0x00000000);

	reg = iosf_usbphy_read(0x4026 + port * 0x100);
	iosf_usbphy_write(0x4026 + port * 0x100, 0x0000001c);
	iosf_usbphy_write(0x4026 + port * 0x100, 0x00000000);
	iosf_usbphy_write(0x4026 + port * 0x100, 0x0000011c);
	iosf_usbphy_write(0x4026 + port * 0x100, reg);

}

static void apply_usb_workaround(void)
{
	usb_workaround_for_port(1);
	usb_workaround_for_port(2);
	usb_workaround_for_port(3);
	usb_workaround_for_port(4);
	usb_workaround_for_port(5);

	/*
	 * If C0 stepping or higher, set the revision in PMC.
	 * Needs to be done before setting BIOS_RESET_DONE and BIOS_ALL_DONE in PUNIT.
	 */
	if (SocStepping() >= SocC0)
		clrsetbits32((void *)(PMC_BASE_ADDRESS + PMC_CRID), PMC_CRID_RID_SEL, 1);
	else
		clrbits32((void *)(PMC_BASE_ADDRESS + PMC_CRID), PMC_CRID_RID_SEL);

	/*
	 * Set BIOS_RESET_DONE and BIOS_ALL_DONE in PUNIT then poll for PUNIT_RTOS_DONE
	 * so that warm reset will work. We need to do a warm reset after USB workaround
	 * otherwise USB 2.0 ports in XHCI will not be able to reset during USB enumeration.
	 */
	iosf_punit_write(PUNIT_BIOS_RESET_CPL,
			 iosf_punit_read(PUNIT_BIOS_RESET_CPL) |
					 BIOS_RESET_DONE | BIOS_ALL_DONE);
	while (!(iosf_punit_read(PUNIT_BIOS_RESET_CPL) & PUNIT_RTOS_DONE));

	system_reset();
}

static bool need_usb_workaround(void)
{
	u32 gen_pmcon1 = read32((void *)(PMC_BASE_ADDRESS + GEN_PMCON1));

	if (gen_pmcon1 & (PWR_FLR | SUS_PWR_FLR)) {
		/*
		 * Clear the failure to avoid reset loop, FSP does not seem to use it
		 * except the TempRamInit API which we not use anymore.
		 */
		printk(BIOS_DEBUG, "GEN_PMCON1: %08x\n", gen_pmcon1);
		write32((void *)(PMC_BASE_ADDRESS + GEN_PMCON1), gen_pmcon1);
		printk(BIOS_DEBUG, "Need USB workaround\n");
		return true;
	}

	return false;
}

void bootblock_soc_early_init(void)
{
	/* Allow memory-mapped PCI config access */
	setup_mmconfig();

	if (!CONFIG(USE_GENERIC_FSP_CAR_INC)) {
		enable_devices();
		set_max_non_turbo_freq();
	}

	/* Early chipset initialization */
	program_base_addresses();
	tco_disable();
}

void bootblock_soc_init(void)
{
	/* Halt if there was a built in self test failure */
	if (!CONFIG(USE_GENERIC_FSP_CAR_INC)) {

		report_bist_failure(saved_bist);
		init_hpet_registers();
		/* This has to be done early */
		if (need_usb_workaround())
			apply_usb_workaround();
	} else {
		report_fsp_output();
	}

	/* Continue chipset initialization */
	soc_rtc_init();
	set_max_freq();
	spi_init();

	lpc_init();
}
