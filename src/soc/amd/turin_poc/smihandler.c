/* SPDX-License-Identifier: GPL-2.0-or-later */

#include <acpi/acpi.h>
#include <amdblocks/acpi.h>
#include <amdblocks/acpimmio.h>
#include <amdblocks/psp.h>
#include <amdblocks/smi.h>
#include <amdblocks/smm.h>
#include <amdblocks/spi.h>
#include <arch/hlt.h>
#include <arch/io.h>
#include <console/console.h>
#include <cpu/x86/cache.h>
#include <cpu/x86/smm.h>
#include <soc/amd/common/block/psp/psp_def.h>
#include <soc/smi.h>
#include <soc/smu.h>
#include <soc/southbridge.h>
#include <types.h>

/*
 * Both the psp_notify_sx_info and the smu_sx_entry call will clobber the SMN index register
 * during the SMN accesses. Since the SMI handler is the last thing that gets called before
 * entering S3, this won't interfere with any indirect SMN accesses via the same register pair.
 */
static void fch_slp_typ_handler(void)
{
	uint32_t pci_ctrl;
	uint16_t pm1cnt;
	uint8_t slp_typ, rst_ctrl;

	/* Figure out SLP_TYP */
	pm1cnt = acpi_read16(MMIO_ACPI_PM1_CNT_BLK);
	printk(BIOS_SPEW, "SMI#: SLP = 0x%04x\n", pm1cnt);
	slp_typ = acpi_sleep_from_pm1(pm1cnt);

	/* Do any mainboard sleep handling */
	mainboard_smi_sleep(slp_typ);

	switch (slp_typ) {
	case ACPI_S0:
		printk(BIOS_DEBUG, "SMI#: Entering S0 (On)\n");
		break;
	case ACPI_S3:
		printk(BIOS_DEBUG, "SMI#: Entering S3 (Suspend-To-RAM)\n");
		break;
	case ACPI_S4:
		printk(BIOS_DEBUG, "SMI#: Entering S4 (Suspend-To-Disk)\n");
		break;
	case ACPI_S5:
		printk(BIOS_DEBUG, "SMI#: Entering S5 (Soft Power off)\n");
		break;
	default:
		printk(BIOS_DEBUG, "SMI#: ERROR: SLP_TYP reserved\n");
		break;
	}

	if (slp_typ >= ACPI_S3) {
		wbinvd();

		clear_all_smi_status();

		/* Do not send SMI before AcpiPm1CntBlkx00[SlpTyp] */
		pci_ctrl = pm_read32(PM_PCI_CTRL);
		pci_ctrl &= ~FORCE_SLPSTATE_RETRY;
		pci_ctrl |= FORCE_STPCLK_RETRY;
		pm_write32(PM_PCI_CTRL, pci_ctrl);

		/* Enable SlpTyp */
		rst_ctrl = pm_read8(PM_RST_CTRL1);
		rst_ctrl |= SLPTYPE_CONTROL_EN;
		pm_write8(PM_RST_CTRL1, rst_ctrl);

		smu_sx_entry(); /* Leave SlpTypeEn clear, SMU will set */
		printk(BIOS_ERR, "System did not go to sleep\n");
		hlt();
	}
}

static void fch_pwrbtn_smi_handler(void)
{
	uint32_t pci_ctrl;
	uint8_t rst_ctrl;

	wbinvd();

	configure_smi(SMITYPE_SLP_TYP, SMI_MODE_DISABLE);

	clear_all_smi_status();
	acpi_write16(MMIO_ACPI_PM1_STS, PWRBTN_STS);

	/* Do not send SMI before AcpiPm1CntBlkx00[SlpTyp] */
	pci_ctrl = pm_read32(PM_PCI_CTRL);
	pci_ctrl &= ~FORCE_SLPSTATE_RETRY;
	pci_ctrl |= FORCE_STPCLK_RETRY;
	pm_write32(PM_PCI_CTRL, pci_ctrl);

	/* Enable SlpTyp */
	rst_ctrl = pm_read8(PM_RST_CTRL1);
	rst_ctrl |= SLPTYPE_CONTROL_EN;
	pm_write8(PM_RST_CTRL1, rst_ctrl);

	/* Leave SlpTypeEn clear, SMU will set */
	set_pm1cnt_s5();
	smu_sx_entry();
	printk(BIOS_ERR, "System did not go to sleep\n");
	hlt();
}

/*
 * Table of functions supported in the SMI handler.  Note that SMI source setup
 * in fch.c is unrelated to this list.
 */
static const struct smi_sources_t smi_sources[] = {
	{ .type = SMITYPE_SMI_CMD_PORT, .handler = fch_apmc_smi_handler },
	{ .type = SMITYPE_SLP_TYP, .handler = fch_slp_typ_handler},
	{ .type = SMITYPE_PSP, .handler = psp_smi_handler },
	{ .type = SMITYPE_PWRBUTTON_UP, .handler = fch_pwrbtn_smi_handler },
};

void *get_smi_source_handler(int source)
{
	size_t i;

	for (i = 0 ; i < ARRAY_SIZE(smi_sources) ; i++)
		if (smi_sources[i].type == source)
			return smi_sources[i].handler;

	return NULL;
}

#define SPI_CMD_READ_ID			0x9f
#define SPI_CMD_READ_ARRAY_SLOW		0x03
#define SPI_CMD_READ_ARRAY_FAST		0x0b
#define SPI_CMD_READ_STATUS		0x05
#define SPI_CMD_WRITE_ENABLE		0x06
#define SPI_CMD_BLOCK_ERASE		0xD8
#define SPI_CMD_PAGE_PROGRAM		0x02
#define SPI_CMD_SECTOR_ERASE		0x20
#define SPI_CMD_SECTOR_ERASE_32K	0x52

static const struct psp_rom_armor1_whitelist rom_armor_whitelist = {
	.allowed_cmd_count = 9,
	.allowed_region_count = 1,
	.allowed_cmds = {
		/* SPI part will be in 4-Byte mode for 3-Byte commands */
		{
			.cs = CHIP_SELECT_1,
			.freq = CONFIG_NORMAL_READ_SPI_SPEED,
			.opcode = SPI_CMD_READ_ID,
			.min_tx = 0,
			.max_tx = 0,
			.min_rx = 3,
			.max_rx = 3,
			.addr_check = NO_ADDR_CHECK,
			.impact_size = 0
		},
		{
			.cs = CHIP_SELECT_1,
			.freq = CONFIG_NORMAL_READ_SPI_SPEED,
			.opcode = SPI_CMD_READ_STATUS,
			.min_tx = 0,
			.max_tx = 0,
			.min_rx = 1,
			.max_rx = 3,
			.addr_check = NO_ADDR_CHECK,
			.impact_size = 0
		},
		{
			.cs = CHIP_SELECT_1,
			.freq = CONFIG_NORMAL_READ_SPI_SPEED,
			.opcode = SPI_CMD_READ_ARRAY_SLOW,
			.min_tx = 4,
			.max_tx = 4,
			.min_rx = 1,
			.max_rx = 68,
			.addr_check = NO_ADDR_CHECK,
			.impact_size = 0
		},
		{
			.cs = CHIP_SELECT_1,
			.freq = CONFIG_NORMAL_READ_SPI_SPEED,
			.opcode = SPI_CMD_READ_ARRAY_FAST,
			.min_tx = 5,
			.max_tx = 5,
			.min_rx = 1,
			.max_rx = 67,
			.addr_check = NO_ADDR_CHECK,
			.impact_size = 0
		},
		{
			.cs = CHIP_SELECT_1,
			.freq = CONFIG_NORMAL_READ_SPI_SPEED,
			.opcode = SPI_CMD_WRITE_ENABLE,
			.min_tx = 0,
			.max_tx = 0,
			.min_rx = 0,
			.max_rx = 0,
			.addr_check = NO_ADDR_CHECK,
			.impact_size = 0
		},
		{
			.cs = CHIP_SELECT_1,
			.freq = CONFIG_NORMAL_READ_SPI_SPEED,
			.opcode = SPI_CMD_PAGE_PROGRAM,
			.min_tx = 5,
			.max_tx = 72,
			.min_rx = 0,
			.max_rx = 0,
			.addr_check = ADDR_CHECK_32BIT,
			.impact_size = 256
		},
		{
			.cs = CHIP_SELECT_1,
			.freq = CONFIG_NORMAL_READ_SPI_SPEED,
			.opcode = SPI_CMD_SECTOR_ERASE,
			.min_tx = 4,
			.max_tx = 4,
			.min_rx = 0,
			.max_rx = 0,
			.addr_check = ADDR_CHECK_32BIT,
			.impact_size = 4 * KiB
		},
		{
			.cs = CHIP_SELECT_1,
			.freq = CONFIG_NORMAL_READ_SPI_SPEED,
			.opcode = SPI_CMD_SECTOR_ERASE_32K,
			.min_tx = 4,
			.max_tx = 4,
			.min_rx = 0,
			.max_rx = 0,
			.addr_check = ADDR_CHECK_32BIT,
			.impact_size = 32 * KiB
		},
		{
			.cs = CHIP_SELECT_1,
			.freq = CONFIG_NORMAL_READ_SPI_SPEED,
			.opcode = SPI_CMD_BLOCK_ERASE,
			.min_tx = 4,
			.max_tx = 4,
			.min_rx = 0,
			.max_rx = 0,
			.addr_check = ADDR_CHECK_32BIT,
			.impact_size = 64 * KiB
		},
		{ 0, 0, 0, 0, 0, 0, 0, 0, 0}, /* empty */
		{ 0, 0, 0, 0, 0, 0, 0, 0, 0}, /* empty */
		{ 0, 0, 0, 0, 0, 0, 0, 0, 0}, /* empty */
		{ 0, 0, 0, 0, 0, 0, 0, 0, 0}, /* empty */
		{ 0, 0, 0, 0, 0, 0, 0, 0, 0}, /* empty */
		{ 0, 0, 0, 0, 0, 0, 0, 0, 0}, /* empty */
		{ 0, 0, 0, 0, 0, 0, 0, 0, 0}, /* empty */
		{ 0, 0, 0, 0, 0, 0, 0, 0, 0}, /* empty */
		{ 0, 0, 0, 0, 0, 0, 0, 0, 0}, /* empty */
		{ 0, 0, 0, 0, 0, 0, 0, 0, 0}, /* empty */
		{ 0, 0, 0, 0, 0, 0, 0, 0, 0}, /* empty */
		{ 0, 0, 0, 0, 0, 0, 0, 0, 0}, /* empty */
		{ 0, 0, 0, 0, 0, 0, 0, 0, 0}, /* empty */
		{ 0, 0, 0, 0, 0, 0, 0, 0, 0}, /* empty */
		{ 0, 0, 0, 0, 0, 0, 0, 0, 0}, /* empty */
		{ 0, 0, 0, 0, 0, 0, 0, 0, 0}, /* empty */
		{ 0, 0, 0, 0, 0, 0, 0, 0, 0}, /* empty */
		{ 0, 0, 0, 0, 0, 0, 0, 0, 0}, /* empty */
		{ 0, 0, 0, 0, 0, 0, 0, 0, 0}, /* empty */
		{ 0, 0, 0, 0, 0, 0, 0, 0, 0}, /* empty */
		{ 0, 0, 0, 0, 0, 0, 0, 0, 0}, /* empty */
		{ 0, 0, 0, 0, 0, 0, 0, 0, 0}, /* empty */
		{ 0, 0, 0, 0, 0, 0, 0, 0, 0}  /* empty */
		},
	.allowed_regions = {
		{ 0x00000000, CONFIG_ROM_SIZE - 1 }, /* whole flash */
		{ 0x00000000, 0x00000000 }, /* empty */
		{ 0x00000000, 0x00000000 }, /* empty */
		{ 0x00000000, 0x00000000 }, /* empty */
		{ 0x00000000, 0x00000000 }, /* empty */
		{ 0x00000000, 0x00000000 }, /* empty */
		{ 0x00000000, 0x00000000 }, /* empty */
		{ 0x00000000, 0x00000000 }, /* empty */
		{ 0x00000000, 0x00000000 }, /* empty */
		{ 0x00000000, 0x00000000 }, /* empty */
		{ 0x00000000, 0x00000000 }, /* empty */
		{ 0x00000000, 0x00000000 }, /* empty */
		{ 0x00000000, 0x00000000 }, /* empty */
		{ 0x00000000, 0x00000000 }, /* empty */
		{ 0x00000000, 0x00000000 }, /* empty */
		{ 0x00000000, 0x00000000 }  /* empty */
	}
};

const struct psp_rom_armor1_whitelist *soc_get_psp_rom_armor_whitelist(void)
{
	return &rom_armor_whitelist;
}

void soc_apmc_finalize(void)
{
	pm_write32(PM_ISACONTROL, pm_read32(PM_ISACONTROL) | PM_LOCK_IOMUX);
	psp_send_generic_command(MBOX_BIOS_CMD_LOCK_FCH_REG, "Locking FCH registers");
	psp_send_generic_command(MBOX_BIOS_CMD_LOCK_FCH_GPIO, "Locking FCH GPIO");
}
