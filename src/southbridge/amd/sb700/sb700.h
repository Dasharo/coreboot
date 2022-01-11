/* SPDX-License-Identifier: GPL-2.0-only */

#ifndef SB700_H
#define SB700_H

#include <types.h>
#include <arch/io.h>
#include <device/device.h>
#include <device/pci_ops.h>

/* Power management index/data registers */
#define BIOSRAM_INDEX	0xcd4
#define BIOSRAM_DATA	0xcd5
#define PM_INDEX	0xcd6
#define PM_DATA		0xcd7
#define PM2_INDEX	0xcd0
#define PM2_DATA	0xcd1

#define SB700_ACPI_IO_BASE 0x800

#define BIOSRAM_AP_ENTRY	0xe8

#define ACPI_PM_EVT_BLK		(SB700_ACPI_IO_BASE + 0x00) /* 4 bytes */
#define ACPI_PM1_CNT_BLK	(SB700_ACPI_IO_BASE + 0x04) /* 2 bytes */
#define ACPI_PMA_CNT_BLK	(SB700_ACPI_IO_BASE + 0x16) /* 1 byte */
#define ACPI_PM_TMR_BLK		(SB700_ACPI_IO_BASE + 0x20) /* 4 bytes */
#define ACPI_GPE0_BLK		(SB700_ACPI_IO_BASE + 0x18) /* 8 bytes */
#define ACPI_CPU_CONTROL	(SB700_ACPI_IO_BASE + 0x08) /* 6 bytes */
#define ACPI_CPU_P_LVL2		(ACPI_CPU_CONTROL + 0x4)    /* 1 byte */

#define SPI_BASE_ADDRESS		((void *)0xfec10000)
#define HPET_BASE_ADDRESS		0xfed00000

#define REV_SB700_A11	0x11
#define REV_SB700_A12	0x12
#define REV_SB700_A14	0x14
#define REV_SB700_A15	0x15

static inline void pmio_write(u8 reg, u8 value)
{
	outb(reg, PM_INDEX);
	outb(value, PM_DATA);
}

static inline u8 pmio_read(u8 reg)
{
	outb(reg, PM_INDEX);
	return inb(PM_DATA);
}

static inline void pmio2_write(u8 reg, u8 value)
{
	outb(reg, PM2_INDEX);
	outb(value, PM2_DATA);
}

static inline u8 pmio2_read(u8 reg)
{
	outb(reg, PM_INDEX);
	return inb(PM2_DATA);
}

void set_sm_enable_bits(struct device *sm_dev, u32 reg_pos, u32 mask, u32 val);

/* This shouldn't be called before set_sb700_revision() is called.
 * Once set_sb700_revision() is called, we use get_sb700_revision(),
 * the simpler one, to get the sb700 revision ID.
 * The id is 0x39 if A11, 0x3A if A12, 0x3C if A14, 0x3D if A15.
 * The differentiate is 0x28, isn't it? */
#define get_sb700_revision(sm_dev)	(pci_read_config8((sm_dev), 0x08) - 0x28)


void sb7xx_51xx_enable_wideio(u8 wio_index, u16 base);
void sb7xx_51xx_disable_wideio(u8 wio_index);
void sb7xx_51xx_early_setup(void);
void sb7xx_51xx_before_pci_init(void);
u16 sb7xx_51xx_decode_last_reset(void);

/* allow override in mainboard.c */
void sb7xx_51xx_setup_sata_port_indication(void *sata_bar5);

#if !ENV_PCI_SIMPLE_DEVICE
void sb7xx_51xx_setup_sata_phys(struct device *dev);
void sb7xx_51xx_enable(struct device *dev);
#endif

void set_lpc_sticky_ctl(bool enable);

int s3_save_nvram_early(u32 dword, int size, int  nvram_pos);
int s3_load_nvram_early(int size, u32 *old_dword, int nvram_pos);

void save_bios_ram_data(u32 dword, int size, int biosram_pos);
void load_bios_ram_data(u32 *dword, int size, int biosram_pos);

void enable_fid_change_on_sb(u32 sbbusn, u32 sbdn);
u32 get_sbdn(u32 bus);

#endif /* SB700_H */
