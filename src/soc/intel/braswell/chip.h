/* SPDX-License-Identifier: GPL-2.0-only */

/*
 * The devicetree parser expects chip.h to reside directly in the path
 * specified by the devicetree.
 */

#ifndef _SOC_CHIP_H_
#define _SOC_CHIP_H_

#include <drivers/intel/gma/i915.h>
#include <fsp/util.h>
#include <intelblocks/lpc_lib.h>
#include <soc/pci_devs.h>
#include <types.h>

#define SVID_CONFIG1		1
#define SVID_CONFIG3		3
#define SVID_PMIC_CONFIG	8

#define IGD_MEMSIZE_32MB	0x01
#define IGD_MEMSIZE_64MB	0x02
#define IGD_MEMSIZE_96MB	0x03
#define IGD_MEMSIZE_128MB	0x04

#define V_PCH_LPC_RID_A0		0x00  // A0 Stepping
#define V_PCH_LPC_RID_A1		0x04  // A1 Stepping
#define V_PCH_LPC_RID_A2		0x08  // A2 Stepping
#define V_PCH_LPC_RID_A3		0x0C  // A3 Stepping
#define V_PCH_LPC_RID_A4		0x80  // A4 Stepping
#define V_PCH_LPC_RID_A5		0x84  // A5 Stepping
#define V_PCH_LPC_RID_A6		0x88  // A6 Stepping
#define V_PCH_LPC_RID_A7		0x8C  // A7 Stepping
#define V_PCH_LPC_RID_B0		0x10  // B0 Stepping
#define V_PCH_LPC_RID_B1		0x14  // B1 Stepping
#define V_PCH_LPC_RID_B2		0x18  // B2 Stepping
#define V_PCH_LPC_RID_B3		0x1C  // B3 Stepping
#define V_PCH_LPC_RID_B4		0x90  // B4 Stepping
#define V_PCH_LPC_RID_B5		0x94  // B5 Stepping
#define V_PCH_LPC_RID_B6		0x98  // B6 Stepping
#define V_PCH_LPC_RID_B7		0x9C  // B7 Stepping
#define V_PCH_LPC_RID_C0		0x20  // C0 Stepping
#define V_PCH_LPC_RID_C1		0x24  // C1 Stepping
#define V_PCH_LPC_RID_C2		0x28  // C2 Stepping
#define V_PCH_LPC_RID_C3		0x2C  // C3 Stepping
#define V_PCH_LPC_RID_C4		0xA0  // C4 Stepping
#define V_PCH_LPC_RID_C5		0xA4  // C5 Stepping
#define V_PCH_LPC_RID_C6		0xA8  // C6 Stepping
#define V_PCH_LPC_RID_C7		0xAC  // C7 Stepping
#define V_PCH_LPC_RID_D0		0x30  // D0 Stepping
#define V_PCH_LPC_RID_D1		0x34  // D1 Stepping
#define V_PCH_LPC_RID_D2		0x38  // D2 Stepping
#define V_PCH_LPC_RID_D3		0x3C  // D3 Stepping
#define V_PCH_LPC_RID_D4		0xB0  // D4 Stepping
#define V_PCH_LPC_RID_D5		0xB4  // D5 Stepping
#define V_PCH_LPC_RID_D6		0xB8  // D6 Stepping
#define V_PCH_LPC_RID_D7		0xBC  // D7 Stepping
#define B_PCH_LPC_RID_STEPPING_MASK	0xFC  // SoC Stepping Mask (Ignoring Package Type)

enum {
	SocA0		= 0,
	SocA1		= 1,
	SocA2		= 2,
	SocA3		= 3,
	SocA4		= 4,
	SocA5		= 5,
	SocA6		= 6,
	SocA7		= 7,
	SocB0		= 8,
	SocB1		= 9,
	SocB2		= 10,
	SocB3		= 11,
	SocB4		= 12,
	SocB5		= 13,
	SocB6		= 14,
	SocB7		= 15,
	SocC0		= 16,
	SocC1		= 17,
	SocC2		= 18,
	SocC3		= 19,
	SocC4		= 20,
	SocC5		= 21,
	SocC6		= 22,
	SocC7		= 23,
	SocD0		= 24,
	SocD1		= 25,
	SocD2		= 26,
	SocD3		= 27,
	SocD4		= 28,
	SocD5		= 29,
	SocD6		= 30,
	SocD7		= 31,
	SocSteppingMax
};

enum lpe_clk_src {
	LPE_CLK_SRC_XTAL,
	LPE_CLK_SRC_PLL,
};

enum usb_comp_bg_value {
	USB_COMP_BG_575_MV = 7,
	USB_COMP_BG_650_MV = 6,
	USB_COMP_BG_550_MV = 5,
	USB_COMP_BG_537_MV = 4,
	USB_COMP_BG_625_MV = 3,
	USB_COMP_BG_700_MV = 2,
	USB_COMP_BG_600_MV = 1,
	USB_COMP_BG_675_MV = 0,
};

struct soc_intel_braswell_config {
	bool enable_xdp_tap;
	bool clkreq_enable;

	enum serirq_mode serirq_mode;

	/* Disable SLP_X stretching after SUS power well loss */
	bool disable_slp_x_stretch_sus_fail;

	/* LPE Audio Clock configuration */
	enum lpe_clk_src lpe_codec_clk_src; /* Both are 19.2MHz */

	/* Native SD Card controller - override controller capabilities */
	uint32_t sdcard_cap_low;
	uint32_t sdcard_cap_high;

	/* Enable devices in ACPI mode */
	bool lpss_acpi_mode;
	bool emmc_acpi_mode;
	bool sd_acpi_mode;
	bool lpe_acpi_mode;

	/* Allow PCIe devices to wake system from suspend */
	bool pcie_wake_enable;

	/* Program USB2_COMPBG register.
	 * [10:7] - select vref to AFE port
	 *  x111 - 575mV, x110 - 650mV, x101 - 550mV, x100 - 537.5mV,
	 *  x011 - 625mV, x010 - 700mV, x001 - 600mV, x000 - 675mV
	 */
	enum usb_comp_bg_value usb_comp_bg;

	/*
	 * The following fields come from fsp_vpd.h .aka. VpdHeader.h.
	 * These are configuration values that are passed to FSP during MemoryInit.
	 */
	uint16_t PcdMrcInitTsegSize;
	uint16_t PcdMrcInitMmioSize;
	uint8_t  PcdMrcInitSpdAddr1;
	uint8_t  PcdMrcInitSpdAddr2;
	uint8_t  PcdIgdDvmt50PreAlloc;
	uint8_t  PcdApertureSize;
	uint8_t  PcdGttSize;
	uint8_t  PcdLegacySegDecode;
	uint8_t  PcdDvfsEnable;
	uint8_t  PcdCaMirrorEn; /* Command Address Mirroring Enabled */

	/*
	 * The following fields come from fsp_vpd.h .aka. VpdHeader.h.
	 * These are configuration values that are passed to FSP during SiliconInit.
	 */
	uint8_t  PcdSdcardMode;
	uint8_t  PcdEnableHsuart0;
	uint8_t  PcdEnableHsuart1;
	uint8_t  PcdEnableAzalia;
	uint8_t  PcdEnableSata;
	uint8_t  PcdEnableXhci;
	uint8_t  PcdEnableLpe;
	uint8_t  PcdEnableDma0;
	uint8_t  PcdEnableDma1;
	uint8_t  PcdEnableI2C0;
	uint8_t  PcdEnableI2C1;
	uint8_t  PcdEnableI2C2;
	uint8_t  PcdEnableI2C3;
	uint8_t  PcdEnableI2C4;
	uint8_t  PcdEnableI2C5;
	uint8_t  PcdEnableI2C6;
	uint8_t  PunitPwrConfigDisable;
	uint8_t  ChvSvidConfig;
	uint8_t  DptfDisable;
	uint8_t  PcdEmmcMode;
	uint8_t  PcdUsb3ClkSsc;
	uint8_t  PcdDispClkSsc;
	uint8_t  PcdSataClkSsc;
	uint8_t  Usb2Port0PerPortPeTxiSet;
	uint8_t  Usb2Port0PerPortTxiSet;
	uint8_t  Usb2Port0IUsbTxEmphasisEn;
	uint8_t  Usb2Port0PerPortTxPeHalf;
	uint8_t  Usb2Port1PerPortPeTxiSet;
	uint8_t  Usb2Port1PerPortTxiSet;
	uint8_t  Usb2Port1IUsbTxEmphasisEn;
	uint8_t  Usb2Port1PerPortTxPeHalf;
	uint8_t  Usb2Port2PerPortPeTxiSet;
	uint8_t  Usb2Port2PerPortTxiSet;
	uint8_t  Usb2Port2IUsbTxEmphasisEn;
	uint8_t  Usb2Port2PerPortTxPeHalf;
	uint8_t  Usb2Port3PerPortPeTxiSet;
	uint8_t  Usb2Port3PerPortTxiSet;
	uint8_t  Usb2Port3IUsbTxEmphasisEn;
	uint8_t  Usb2Port3PerPortTxPeHalf;
	uint8_t  Usb2Port4PerPortPeTxiSet;
	uint8_t  Usb2Port4PerPortTxiSet;
	uint8_t  Usb2Port4IUsbTxEmphasisEn;
	uint8_t  Usb2Port4PerPortTxPeHalf;
	uint8_t  Usb3Lane0Ow2tapgen2deemph3p5;
	uint8_t  Usb3Lane1Ow2tapgen2deemph3p5;
	uint8_t  Usb3Lane2Ow2tapgen2deemph3p5;
	uint8_t  Usb3Lane3Ow2tapgen2deemph3p5;
	uint8_t  PcdSataInterfaceSpeed;
	uint8_t  PcdPchUsbSsicPort;
	uint8_t  PcdPchUsbHsicPort;
	uint8_t  PcdPcieRootPortSpeed;
	uint8_t  PcdPchSsicEnable;
	uint32_t PcdLogoPtr;
	uint32_t PcdLogoSize;
	uint8_t  PcdRtcLock;
	uint8_t  PMIC_I2CBus;
	uint8_t  ISPEnable;
	uint8_t  ISPPciDevConfig;
	uint8_t  PcdSdDetectChk; /* Enable / Disable SD Card Detect Simulation */
	uint8_t  I2C0Frequency;  /* 0 - 100KHz, 1 - 400KHz, 2 - 1MHz */
	uint8_t  I2C1Frequency;
	uint8_t  I2C2Frequency;
	uint8_t  I2C3Frequency;
	uint8_t  I2C4Frequency;
	uint8_t  I2C5Frequency;
	uint8_t  I2C6Frequency;

	struct i915_gpu_controller_info gfx;
};

int SocStepping(void);

#endif /* _SOC_CHIP_H_ */
