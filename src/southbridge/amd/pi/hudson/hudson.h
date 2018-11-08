/*
 * This file is part of the coreboot project.
 *
 * Copyright (C) 2010 Advanced Micro Devices, Inc.
 * Copyright (C) 2014 Sage Electronic Engineering, LLC
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
 */

#ifndef HUDSON_H
#define HUDSON_H

#include <device/pci_ids.h>
#include <device/device.h>
#include "chip.h"
#include "pci_devs.h"

/* Power management index/data registers */
#define BIOSRAM_INDEX	0xcd4
#define BIOSRAM_DATA	0xcd5
#define PM_INDEX	0xcd6
#define PM_DATA		0xcd7
#define PM2_INDEX	0xcd0
#define PM2_DATA	0xcd1

#define HUDSON_ACPI_IO_BASE 0x800

#define ACPI_PM_EVT_BLK		(HUDSON_ACPI_IO_BASE + 0x00) /* 4 bytes */
#define ACPI_PM1_CNT_BLK	(HUDSON_ACPI_IO_BASE + 0x04) /* 2 bytes */
#define ACPI_PM_TMR_BLK		(HUDSON_ACPI_IO_BASE + 0x18) /* 4 bytes */
#define ACPI_GPE0_BLK		(HUDSON_ACPI_IO_BASE + 0x10) /* 8 bytes */
#define ACPI_CPU_CONTROL	(HUDSON_ACPI_IO_BASE + 0x08) /* 6 bytes */

#define ACPI_SMI_CTL_PORT		0xb2
#define ACPI_SMI_CMD_CST_CONTROL	0xde
#define ACPI_SMI_CMD_PST_CONTROL	0xad
#define ACPI_SMI_CMD_DISABLE		0xbe
#define ACPI_SMI_CMD_ENABLE		0xef
#define ACPI_SMI_CMD_S4_REQ		0xc0

#define REV_HUDSON_A11	0x11
#define REV_HUDSON_A12	0x12

#define SPIROM_BASE_ADDRESS_REGISTER  0xA0
#define SPI_ROM_ENABLE                0x02
#define SPI_BASE_ADDRESS              0xFEC10000

#define LPC_IO_OR_MEM_DECODE_ENABLE	0x48

#define ACPI_MMIO_BASE  0xFED80000ul
#define FCH_CFG_BASE     0x000   // DWORD
#define GPIO_BASE       0x100   // BYTE
//#define SMI_BASE        0x200   // DWORD
#define PMIO_BASE       0x300   // DWORD
#define PMIO2_BASE      0x400   // BYTE
#define BIOS_RAM_BASE   0x500   // BYTE
#define CMOS_RAM_BASE   0x600   // BYTE
#define CMOS_BASE       0x700   // BYTE
#define ASF_BASE        0x900   // DWORD
#define SMBUS_BASE      0xA00   // DWORD
#define WATCHDOG_BASE   0xB00   //
#define HPET_BASE       0xC00   // DWORD
#define IOMUX_BASE      0xD00   // BYTE
#define MISC_BASE       0xE00
#define SERIAL_DEBUG_BASE  0x1000
#define GFX_DAC_BASE       0x1400
#define GPIO_BANK0_BASE    0x1500   // DWORD
//#define GPIO_BANK1_BASE    0x1600   // DWORD
//#define GPIO_BANK2_BASE    0x1700   // DWORD
#define CEC_BASE           0x1800
#define XHCI_BASE          0x1C00
#define ACDC_BASE          0x1D00
#define AOAC_BASE          0x1E00


// FCH MISC REGISTERS
#define FCH_MISC_REG28           0x28         //  ClkDrvSth2
#define FCH_MISC_REG2C           0x2C
#define FCH_MISC_REG40           0x40         //  MiscCntrl      for clock only
#define FCH_MISC_REG41           0x41         //  MiscCntr2
#define FCH_MISC_REG42           0x42         //  MiscCntr3
#define FCH_MISC_REG44           0x44         //  ValueOnPort80


//
// Fan control parameters
//

// PM2x00 Fan0InputControl ** THIS SHOULD BE WRITTEN LAST IT TRIGGERS THE NEW PARAMETERS TO BE USED
// FanInputControl (2:0)
#define PM2_FAN_0_INPUT_CONTROL		0
#define FAN_INPUT_INTERNAL_DIODE	0
#define FAN_INPUT_TEMP0				1
#define FAN_INPUT_TEMP1				2
#define FAN_INPUT_TEMP2				3
#define FAN_INPUT_TEMP3				4
#define FAN_INPUT_TEMP0_FILTER		5
#define FAN_INPUT_ZERO				6
#define FAN_INPUT_DISABLED			7
// TwoRampAlorithmEn (3)
#define FAN_TWO_RAMP_ALGO_EN		(1 << 3)

// PM2x01 Fan0Control
#define PM2_FAN_0_CONTROL		1
#define FAN_AUTOMODE			(1 << 0)
#define FAN_LINEARMODE			(1 << 1)
#define FAN_STEPMODE			~(1 << 1)
#define FAN_POLARITY_HIGH		(1 << 2)
#define FAN_POLARITY_LOW		~(1 << 2)

// PM2x02 Fan0Freq
/* Normally, 4-wire fan runs at 25KHz and 3-wire fan runs at 100Hz */
#define PM2_FAN_0_FREQ			2
#define FREQ_28KHZ			0x0
#define FREQ_25KHZ			0x1
#define FREQ_23KHZ			0x2
#define FREQ_21KHZ			0x3
#define FREQ_29KHZ			0x4
#define FREQ_18KHZ			0x5
#define FREQ_100HZ			0xF7
#define FREQ_87HZ			0xF8
#define FREQ_58HZ			0xF9
#define FREQ_44HZ			0xFA
#define FREQ_35HZ			0xFB
#define FREQ_29HZ			0xFC
#define FREQ_22HZ			0xFD
#define FREQ_14HZ			0xFE
#define FREQ_11HZ			0xFF

// PM2x03 LowDuty0
// 0 = STOP 0xFF = FULL SPEED
#define PM2_FAN_0_LOW_DUTY			3

// PM2x04 MedDuty0
// 0 = STOP 0xFF = FULL SPEED
#define PM2_FAN_0_MED_DUTY			4

#define PM_RTC_CONTROL		0x56
#define PM_S_STATE_CONTROL	0xBA


static inline int hudson_sata_enable(void)
{
	/* True if IDE or AHCI. */
	return (CONFIG_HUDSON_SATA_MODE == 0) || (CONFIG_HUDSON_SATA_MODE == 2);
}

static inline int hudson_ide_enable(void)
{
	/* True if IDE or LEGACY IDE. */
	return (CONFIG_HUDSON_SATA_MODE == 0) || (CONFIG_HUDSON_SATA_MODE == 3);
}

#ifndef __SMM__

void pm_write8(u8 reg, u8 value);
u8 pm_read8(u8 reg);
void pm_write16(u8 reg, u16 value);
u16 pm_read16(u16 reg);

void pm2_write8(u8 reg, u8 value);
u8 pm2_read8(u8 reg);

#ifdef __PRE_RAM__
void hudson_lpc_port80(void);
void hudson_pci_port80(void);
void hudson_clk_output_48Mhz(void);

int s3_save_nvram_early(u32 dword, int size, int  nvram_pos);
int s3_load_nvram_early(int size, u32 *old_dword, int nvram_pos);

#else
void hudson_enable(device_t dev);
void s3_resume_init_data(void *FchParams);

#endif /* __PRE_RAM__ */
#endif /* __SMM__ */


#ifndef BIT0
  #define BIT0        0x0000000000000001ull
#endif
#ifndef BIT1
  #define BIT1        0x0000000000000002ull
#endif
#ifndef BIT2
  #define BIT2        0x0000000000000004ull
#endif
#ifndef BIT3
  #define BIT3        0x0000000000000008ull
#endif
#ifndef BIT4
  #define BIT4        0x0000000000000010ull
#endif
#ifndef BIT5
  #define BIT5        0x0000000000000020ull
#endif
#ifndef BIT6
  #define BIT6        0x0000000000000040ull
#endif
#ifndef BIT7
  #define BIT7        0x0000000000000080ull
#endif
#ifndef BIT8
  #define BIT8        0x0000000000000100ull
#endif
#ifndef BIT9
  #define BIT9        0x0000000000000200ull
#endif
#ifndef BIT10
  #define BIT10       0x0000000000000400ull
#endif
#ifndef BIT11
  #define BIT11       0x0000000000000800ull
#endif
#ifndef BIT12
  #define BIT12       0x0000000000001000ull
#endif
#ifndef BIT13
  #define BIT13       0x0000000000002000ull
#endif
#ifndef BIT14
  #define BIT14       0x0000000000004000ull
#endif
#ifndef BIT15
  #define BIT15       0x0000000000008000ull
#endif
#ifndef BIT16
  #define BIT16       0x0000000000010000ull
#endif
#ifndef BIT17
  #define BIT17       0x0000000000020000ull
#endif
#ifndef BIT18
  #define BIT18       0x0000000000040000ull
#endif
#ifndef BIT19
  #define BIT19       0x0000000000080000ull
#endif
#ifndef BIT20
  #define BIT20       0x0000000000100000ull
#endif
#ifndef BIT21
  #define BIT21       0x0000000000200000ull
#endif
#ifndef BIT22
  #define BIT22       0x0000000000400000ull
#endif
#ifndef BIT23
  #define BIT23       0x0000000000800000ull
#endif
#ifndef BIT24
  #define BIT24       0x0000000001000000ull
#endif
#ifndef BIT25
  #define BIT25       0x0000000002000000ull
#endif
#ifndef BIT26
  #define BIT26       0x0000000004000000ull
#endif
#ifndef BIT27
  #define BIT27       0x0000000008000000ull
#endif
#ifndef BIT28
  #define BIT28       0x0000000010000000ull
#endif
#ifndef BIT29
  #define BIT29       0x0000000020000000ull
#endif
#ifndef BIT30
  #define BIT30       0x0000000040000000ull
#endif
#ifndef BIT31
  #define BIT31       0x0000000080000000ull
#endif
#ifndef BIT32
  #define BIT32       0x0000000100000000ull
#endif
#ifndef BIT33
  #define BIT33       0x0000000200000000ull
#endif
#ifndef BIT34
  #define BIT34       0x0000000400000000ull
#endif
#ifndef BIT35
  #define BIT35       0x0000000800000000ull
#endif
#ifndef BIT36
  #define BIT36       0x0000001000000000ull
#endif
#ifndef BIT37
  #define BIT37       0x0000002000000000ull
#endif
#ifndef BIT38
  #define BIT38       0x0000004000000000ull
#endif
#ifndef BIT39
  #define BIT39       0x0000008000000000ull
#endif
#ifndef BIT40
  #define BIT40       0x0000010000000000ull
#endif
#ifndef BIT41
  #define BIT41       0x0000020000000000ull
#endif
#ifndef BIT42
  #define BIT42       0x0000040000000000ull
#endif
#ifndef BIT43
  #define BIT43       0x0000080000000000ull
#endif
#ifndef BIT44
  #define BIT44       0x0000100000000000ull
#endif
#ifndef BIT45
  #define BIT45       0x0000200000000000ull
#endif
#ifndef BIT46
  #define BIT46       0x0000400000000000ull
#endif
#ifndef BIT47
  #define BIT47       0x0000800000000000ull
#endif
#ifndef BIT48
  #define BIT48       0x0001000000000000ull
#endif
#ifndef BIT49
  #define BIT49       0x0002000000000000ull
#endif
#ifndef BIT50
  #define BIT50       0x0004000000000000ull
#endif
#ifndef BIT51
  #define BIT51       0x0008000000000000ull
#endif
#ifndef BIT52
  #define BIT52       0x0010000000000000ull
#endif
#ifndef BIT53
  #define BIT53       0x0020000000000000ull
#endif
#ifndef BIT54
  #define BIT54       0x0040000000000000ull
#endif
#ifndef BIT55
  #define BIT55       0x0080000000000000ull
#endif
#ifndef BIT56
  #define BIT56       0x0100000000000000ull
#endif
#ifndef BIT57
  #define BIT57       0x0200000000000000ull
#endif
#ifndef BIT58
  #define BIT58       0x0400000000000000ull
#endif
#ifndef BIT59
  #define BIT59       0x0800000000000000ull
#endif
#ifndef BIT60
  #define BIT60       0x1000000000000000ull
#endif
#ifndef BIT61
  #define BIT61       0x2000000000000000ull
#endif
#ifndef BIT62
  #define BIT62       0x4000000000000000ull
#endif
#ifndef BIT63
  #define BIT63       0x8000000000000000ull
#endif

#endif /* HUDSON_H */
