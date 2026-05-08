/* amdtool: dump AMD Promontory 21 chipset registers */
/* SPDX-License-Identifier: GPL-2.0-only */

/*
 * Register addresses and access methods derived from:
 * src/vendorcode/amd/opensil/phoenix_poc/opensil/xUSL/PROM/
 */

#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include "amdtool.h"

/* Promontory 21 PCI Device IDs (AMD vendor 0x1022) */
#define PT21_USP_DID        0x43F4
#define PT21_SATA_DID       0x43F6
#define PT21_XHCI_DID_MIN   0x43F7
#define PT21_XHCI_DID_MAX   0x43FE

/*
 * Indirect register access via xHCI MMIO offsets.
 * Write 3-byte address (MSB first), then read/write data byte.
 * Status bit 7 = busy; poll until clear after each address byte write.
 */
#define PT21_IND_ADDR2       0x3002
#define PT21_IND_ADDR1       0x3001
#define PT21_IND_ADDR0       0x3000
#define PT21_IND_DATA_R      0x3008
#define PT21_IND_DATA_W      0x3004
#define PT21_IND_STATUS      0x3009
#define PT21_IND_BUSY        0x80

/* Promontory 21 internal (indirect) register addresses */

/* USB settings */
#define PT21_PME_REG         0x18515
#define PT21_DBC_REG         0x18A61
/* USB port control bitmaps (1 bit per port, 0=enabled, 1=disabled) */
#define PT21_USB_PORT_USB3   0x1C51C	/* USB3 ports 0-5 */
#define PT21_USB_PORT_USB2_L 0x1C51D	/* USB2 ports 0-7 */
#define PT21_USB_PORT_USB2_H 0x1C51E	/* USB2 ports 8-11 */

/* PCIe / general configuration */
/*
 * Access gate for the 0x247xx register bank: write 0x86 to unlock,
 * 0x00 to lock. Applies to DSP enable, SSID, CLKREQ, and SATA port regs.
 */
#define PT21_BANK_GATE       0x24788
#define PT21_BANK_UNLOCK     0x86
#define PT21_BANK_LOCK       0x00
#define PT21_DSP_ENABLE      0x24734	/* 16-bit bitmap, bit N = DSP port N enabled */
#define PT21_CLKREQ_MODE     0x24720	/* 16-bit, 2 bits per CLKREQ: mode */
#define PT21_CLKREQ_PINSEL   0x24724	/* 32-bit, 4 bits per CLKREQ: pin select */
/*
 * Lane reversal per DSP port: PORM21_LANE_REVERSAL_REG(a) = 0x28003 + a*0x400
 * Valid for DSP ports 0..10 (PROM21_MAX_LANE_REVERSAL_DSP_NUMBER = 11)
 */
#define PT21_LANE_REV_BASE   0x28003
#define PT21_LANE_REV_STEP   0x400
/* PCIe Gen1 TX swing per lane: 0x2C001 + lane*0x100, lanes 0..11 */
#define PT21_GEN1_SWING_BASE 0x2C001
#define PT21_GEN1_SWING_STEP 0x100
/* Thermal throttle control */
#define PT21_THERMAL_CTRL    0x1C51F	/* bit1=throttle en, bit2=gen1 applied, bit7=SI applied */
#define PT21_THERMAL_THRESH  0x1E521

/* SATA */
#define PT21_SATA_CTRL_REG   0x10151	/* bit1=SATA enabled */
#define PT21_SATA_PORT_EN1   0x2471C	/* gated: byte bitmap, bit N = port N enabled */
#define PT21_SATA_PORT_EN2   0x2E006	/* ungated: byte bitmap */
#define PT21_SATA_CLKREQ     0x24720
#define PT21_SATA_CAP        0x2E000	/* 32-bit AHCI CAP register */
#define PT21_SATA_DEVSLP     0x2E00A	/* byte bitmap: bit N = port N DevSleep enabled */
#define PT21_SATA_HOTPLUG0   0x2E0C8	/* +PortNum for ports 0-3, bit3=hotplug enabled */
#define PT21_AHCI_MSI_CAP    0x23C34	/* 0x50=MSI enabled, 0x70=disabled */

/* eFUSE / silicon revision */
#define PT21_EFUSE_REG       0x2E37B	/* bit3=1 - A2, else A0/A1 */

/* xHCI MSI and MSI-X capability header pointer register */
#define PT21_XHCI_MSI_CAP    0x23834	/* 0x50=MSI enabled, 0x68=disabled */
#define PT21_XHCI_MSIX_CAP   0x23851	/* 0x68=MSI-X enabled, 0x78=disabled */
/*
 * EqPreset register: bits[1:0] hold PT21EqPreset.
 * Bit6 is also used by newer firmware as a load-done flag.
 */
#define PT21_EQ_PRESET_REG   0x1E51C	/* bits[1:0] = PT21EqPreset */
/* SATA device ID (16-bit) - behind the bank access gate */
#define PT21_SATA_DID_REG    0x24706	/* SATA function device ID */
#define PT21_SATA_DID_RAID   0x43BD	/* RAID mode device ID */

#define PT21_NUM_USB3_PORTS        6
#define PT21_NUM_USB2_PORT_GROUPS  3	/* 2 ports per group */
#define PT21_NUM_HW_LPM            3
#define PT21_NUM_SATA_PORTS        4
#define PT21_NUM_PCIE_LANES        12
#define PT21_NUM_PCIE_CLKREQ       6

#define PT21_XHCI_MMIO_SIZE   0x4000
#define PT21_GPIO_MMIO_SIZE   0x20

static const uint32_t hw_lpm_en[PT21_NUM_HW_LPM] = {
	0x1A58C, 0x1C58C, 0x1E58C
};

static const uint32_t usb3_gen1_swing[PT21_NUM_USB3_PORTS] = {
	0x19490, 0x1A490, 0x1B490, 0x1C490, 0x1D490, 0x1E490
};

static const uint32_t usb3_gen1_ep[PT21_NUM_USB3_PORTS] = {
	0x19250, 0x1A250, 0x1B250, 0x1C250, 0x1D250, 0x1E250
};

static const uint32_t usb3_gen2_swing[PT21_NUM_USB3_PORTS] = {
	0x194A0, 0x1A4A0, 0x1B4A0, 0x1C4A0, 0x1D4A0, 0x1E4A0
};

static const uint32_t usb3_gen2_cp0_ep[PT21_NUM_USB3_PORTS] = {
	0x19252, 0x1A252, 0x1B252, 0x1C252, 0x1D252, 0x1E252
};

static const uint32_t usb3_gen2_cp13_ep[PT21_NUM_USB3_PORTS] = {
	0x1925C, 0x1A25C, 0x1B25C, 0x1C25C, 0x1D25C, 0x1E25C
};

static const uint32_t usb3_gen2_cp14_ep[PT21_NUM_USB3_PORTS] = {
	0x1925E, 0x1A25E, 0x1B25E, 0x1C25E, 0x1D25E, 0x1E25E
};

static const uint32_t usb3_gen2_cp15_ep[PT21_NUM_USB3_PORTS] = {
	0x19260, 0x1A260, 0x1B260, 0x1C260, 0x1D260, 0x1E260
};

static const uint32_t usb3_gen2_cp16_ep[PT21_NUM_USB3_PORTS] = {
	0x19262, 0x1A262, 0x1B262, 0x1C262, 0x1D262, 0x1E262
};

/*
 * USB2 TX regs: 4 entries per port-group (slew, drive, drive-dup, term).
 * The duplicate at index 2/6/10 mirrors the driving-current register.
 */
static const uint32_t usb2_tx_reg[12] = {
	0x1A598, 0x1A599, 0x1A598, 0x1A59A,
	0x1C598, 0x1C599, 0x1C598, 0x1C59A,
	0x1E598, 0x1E599, 0x1E598, 0x1E59A
};

static const uint32_t sata_gen12_swing[PT21_NUM_SATA_PORTS] = {
	0x2D188, 0x2D388, 0x2D588, 0x2D788
};

static const uint32_t sata_gen3_swing[PT21_NUM_SATA_PORTS] = {
	0x2D189, 0x2D389, 0x2D589, 0x2D789
};

static const uint32_t sata_gen1_emph[PT21_NUM_SATA_PORTS] = {
	0x2D18B, 0x2D38B, 0x2D58B, 0x2D78B
};

static const uint32_t sata_gen2_emph[PT21_NUM_SATA_PORTS] = {
	0x2D18C, 0x2D38C, 0x2D58C, 0x2D78C
};

static const uint32_t sata_gen3_emph[PT21_NUM_SATA_PORTS] = {
	0x2D18D, 0x2D38D, 0x2D58D, 0x2D78D
};

static const uint32_t sata_speed_reg[PT21_NUM_SATA_PORTS] = {
	0x2D12B, 0x2D32B, 0x2D52B, 0x2D72B
};

/*
 * USB3 port speed-select registers (r_force_superspeed in Prom21.h):
 *   bit2      = force_function_enable
 *   bits[2:0]: 0x3 = hw default (no force)
 *              0x4 = force Gen1x1, 0x5 = force Gen1x2
 *              0x6 = force Gen2x1, 0x7 = force Gen2x2
 */
static const uint32_t usb3_force_ss[PT21_NUM_USB3_PORTS] = {
	0x19211, 0x1A211, 0x1B211, 0x1C211, 0x1D211, 0x1E211
};

static void pt21_wait_ready(uint8_t *mmio)
{
	unsigned int retries = 10000;

	while ((read8(mmio + PT21_IND_STATUS) & PT21_IND_BUSY) && retries--)
		;
}

static void pt21_set_addr(uint8_t *mmio, uint32_t addr)
{
	write8(mmio + PT21_IND_ADDR2, (addr >> 16) & 0xFF);
	pt21_wait_ready(mmio);
	write8(mmio + PT21_IND_ADDR1, (addr >> 8) & 0xFF);
	pt21_wait_ready(mmio);
	write8(mmio + PT21_IND_ADDR0, addr & 0xFF);
	pt21_wait_ready(mmio);
}

/*
 * Read one byte from a Promontory 21 internal register via the indirect
 * access mechanism at xHCI MMIO offsets 0x3000-0x3009.
 * Matches Prom21XhciReadByte() in PromAccess.c.
 */
static uint8_t pt21_read_byte(uint8_t *mmio, uint32_t addr)
{
	(void)read8(mmio + PT21_IND_STATUS);	/* dummy status read */
	pt21_set_addr(mmio, addr);
	return read8(mmio + PT21_IND_DATA_R);
}

/* Read a 16-bit little-endian value via two consecutive byte reads. */
static uint16_t pt21_read_word(uint8_t *mmio, uint32_t addr)
{
	uint16_t val;

	(void)read8(mmio + PT21_IND_STATUS);
	pt21_set_addr(mmio, addr);
	val = read8(mmio + PT21_IND_DATA_R);
	pt21_set_addr(mmio, addr + 1);
	val |= (uint16_t)read8(mmio + PT21_IND_DATA_R) << 8;
	return val;
}

/* Read a 32-bit little-endian value via four consecutive byte reads. */
static uint32_t pt21_read_dword(uint8_t *mmio, uint32_t addr)
{
	uint32_t val = 0;
	int i;

	(void)read8(mmio + PT21_IND_STATUS);
	for (i = 0; i < 4; i++) {
		pt21_set_addr(mmio, addr + i);
		val |= (uint32_t)read8(mmio + PT21_IND_DATA_R) << (8 * i);
	}
	return val;
}

/*
 * Write one byte to a Promontory 21 internal register.
 * Used only for the access-gate toggle (0x24788), not for configuration writes.
 * Matches Prom21XhciWriteByte() in PromAccess.c.
 */
static void pt21_write_byte(uint8_t *mmio, uint32_t addr, uint8_t data)
{
	(void)read8(mmio + PT21_IND_STATUS);
	pt21_set_addr(mmio, addr);
	write8(mmio + PT21_IND_DATA_W, data);
	pt21_wait_ready(mmio);
}

static const char *decode_xhci_port_gen(uint8_t val)
{
	switch (val & 0x7) {
	case 0x3: return "hw default";
	case 0x4: return "force Gen1x1";
	case 0x5: return "force Gen1x2";
	case 0x6: return "force Gen2x1";
	case 0x7: return "force Gen2x2";
	default:  return "unknown";
	}
}

static void dump_usb_settings(uint8_t *mmio)
{
	int i;
	uint8_t val;

	printf("\n--- USB Settings ---\n");

	val = pt21_read_byte(mmio, PT21_PME_REG);
	printf("  PME Control     (0x%05x): 0x%02x  PME %s\n",
	       PT21_PME_REG, val, (val & 0x80) ? "disabled" : "enabled");

	val = pt21_read_byte(mmio, PT21_DBC_REG);
	printf("  DbC Control     (0x%05x): 0x%02x\n", PT21_DBC_REG, val);

	printf("  HW LPM:\n");
	for (i = 0; i < PT21_NUM_HW_LPM; i++) {
		val = pt21_read_byte(mmio, hw_lpm_en[i]);
		printf("    Group %d (0x%05x): 0x%02x  bits[4:1]=0x%x\n",
		       i, hw_lpm_en[i], val, (val >> 1) & 0xF);
	}

	/*
	 * Register 0x1C51C layout (USBGen2by1_port_mapping[] in Prom21.h):
	 *   bit0      = USB3 GenSelect (PT21Usb3GenSelect)
	 *   bits[6:1] = USB3 port 0-5 disable flags (1=disabled, 0=enabled)
	 */
	val = pt21_read_byte(mmio, PT21_USB_PORT_USB3);
	printf("  USB3 ports 0-5  (0x%05x): 0x%02x"
	       "  (bit0=GenSelect, bits[6:1]=disabled mask)\n",
	       PT21_USB_PORT_USB3, val);
	printf("    GenSelect (bit0): 0x%x\n", val & 0x01);
	for (i = 0; i < PT21_NUM_USB3_PORTS; i++)
		printf("    USB3 Port %d (bit%d): %s\n", i, i + 1,
		       (val & (1 << (i + 1))) ? "disabled" : "enabled");

	val = pt21_read_byte(mmio, PT21_USB_PORT_USB2_L);
	printf("  USB2 ports 0-7  (0x%05x): 0x%02x\n", PT21_USB_PORT_USB2_L, val);
	for (i = 0; i < 8; i++)
		printf("    USB2 Port %d: %s\n", i,
		       (val & (1 << i)) ? "disabled" : "enabled");

	val = pt21_read_byte(mmio, PT21_USB_PORT_USB2_H);
	printf("  USB2 ports 8-11 (0x%05x): 0x%02x\n", PT21_USB_PORT_USB2_H, val);
	for (i = 0; i < 4; i++)
		printf("    USB2 Port %d: %s\n", 8 + i,
		       (val & (1 << i)) ? "disabled" : "enabled");

	/*
	 * USB3 port speed: r_force_superspeed[] registers.
	 * bit2=force_function_enable; bits[2:0]: 3=hw default,
	 * 4=Gen1x1, 5=Gen1x2, 6=Gen2x1, 7=Gen2x2.
	 */
	printf("  USB3 Port Speed:\n");
	for (i = 0; i < PT21_NUM_USB3_PORTS; i++) {
		val = pt21_read_byte(mmio, usb3_force_ss[i]);
		printf("    Port %d (0x%05x): 0x%02x  %s\n",
		       i, usb3_force_ss[i], val,
		       decode_xhci_port_gen(val));
	}

	/* EqPreset: bits[1:0] of 0x1E51C */
	val = pt21_read_byte(mmio, PT21_EQ_PRESET_REG);
	printf("  EqPreset        (0x%05x): 0x%02x  bits[1:0]=0x%x\n",
	       PT21_EQ_PRESET_REG, val, val & 0x3);

	/* xHCI MSI and MSI-X capability header pointer */
	val = pt21_read_byte(mmio, PT21_XHCI_MSI_CAP);
	printf("  xHCI MSI        (0x%05x): 0x%02x  MSI %s\n",
	       PT21_XHCI_MSI_CAP, val, (val == 0x50) ? "enabled" : "disabled");
	val = pt21_read_byte(mmio, PT21_XHCI_MSIX_CAP);
	printf("  xHCI MSI-X      (0x%05x): 0x%02x  MSI-X %s\n",
	       PT21_XHCI_MSIX_CAP, val, (val == 0x68) ? "enabled" : "disabled");
}

static void dump_usb3_phy(uint8_t *mmio)
{
	int i;

	printf("\n--- USB3 PHY Tuning ---\n");
	for (i = 0; i < PT21_NUM_USB3_PORTS; i++) {
		printf("  Port %d:\n", i);
		printf("    Gen1 Swing          (0x%05x): 0x%02x\n",
		       usb3_gen1_swing[i], pt21_read_byte(mmio, usb3_gen1_swing[i]));
		/*
		 * Emphasis/preshoot registers are 16-bit words:
		 *   bits[7:4] = EmpLevel, bit3 = PreshootEn, bits[2:0] = Preshoot
		 *   bit8      = EmpLevelEn  (PT21USB3PortGen*EmpLevelEn)
		 * Read as word so EmpLevelEn is captured in the upper byte.
		 */
		printf("    Gen1 Emph/Preshoot  (0x%05x): 0x%04x\n",
		       usb3_gen1_ep[i], pt21_read_word(mmio, usb3_gen1_ep[i]));
		printf("    Gen2 Swing          (0x%05x): 0x%02x\n",
		       usb3_gen2_swing[i], pt21_read_byte(mmio, usb3_gen2_swing[i]));
		printf("    Gen2 CP0  Emph/Pre  (0x%05x): 0x%04x\n",
		       usb3_gen2_cp0_ep[i], pt21_read_word(mmio, usb3_gen2_cp0_ep[i]));
		printf("    Gen2 CP13 Emph/Pre  (0x%05x): 0x%04x\n",
		       usb3_gen2_cp13_ep[i], pt21_read_word(mmio, usb3_gen2_cp13_ep[i]));
		printf("    Gen2 CP14 Emph/Pre  (0x%05x): 0x%04x\n",
		       usb3_gen2_cp14_ep[i], pt21_read_word(mmio, usb3_gen2_cp14_ep[i]));
		printf("    Gen2 CP15 Emph/Pre  (0x%05x): 0x%04x\n",
		       usb3_gen2_cp15_ep[i], pt21_read_word(mmio, usb3_gen2_cp15_ep[i]));
		printf("    Gen2 CP16 Emph/Pre  (0x%05x): 0x%04x\n",
		       usb3_gen2_cp16_ep[i], pt21_read_word(mmio, usb3_gen2_cp16_ep[i]));
	}
}

static void dump_usb2_phy(uint8_t *mmio)
{
	int i;

	printf("\n--- USB2 PHY Tuning ---\n");
	for (i = 0; i < PT21_NUM_USB2_PORT_GROUPS; i++) {
		printf("  Port group %d (ports %d-%d):\n", i, i * 2, i * 2 + 1);
		printf("    SlewRate       (0x%05x): 0x%02x\n",
		       usb2_tx_reg[i * 4 + 0],
		       pt21_read_byte(mmio, usb2_tx_reg[i * 4 + 0]));
		printf("    DrivingCurrent (0x%05x): 0x%02x\n",
		       usb2_tx_reg[i * 4 + 1],
		       pt21_read_byte(mmio, usb2_tx_reg[i * 4 + 1]));
		printf("    Termination    (0x%05x): 0x%02x\n",
		       usb2_tx_reg[i * 4 + 3],
		       pt21_read_byte(mmio, usb2_tx_reg[i * 4 + 3]));
	}
}

static void dump_pcie_config(uint8_t *mmio)
{
	int i;
	uint16_t dsp_en, clkreq_mode;
	uint32_t clkreq_pinsel;
	uint8_t val;

	printf("\n--- PCIe DSP Configuration ---\n");

	/*
	 * The 0x247xx register bank requires the access gate to be unlocked
	 * (write 0x86 to 0x24788) before reading, and locked again after
	 * (write 0x00). This matches the firmware sequence in Prom21.c /
	 * Prom21Sata.c for Prom21DSPortSetting / Prom21GppClockOutput.
	 */
	pt21_write_byte(mmio, PT21_BANK_GATE, PT21_BANK_UNLOCK);
	dsp_en      = pt21_read_word(mmio, PT21_DSP_ENABLE);
	clkreq_mode = pt21_read_word(mmio, PT21_CLKREQ_MODE);
	clkreq_pinsel = pt21_read_dword(mmio, PT21_CLKREQ_PINSEL);
	pt21_write_byte(mmio, PT21_BANK_GATE, PT21_BANK_LOCK);

	printf("  DSP Port Enable (0x%05x): 0x%04x\n", PT21_DSP_ENABLE, dsp_en);
	for (i = 0; i <= 12; i++)
		printf("    DSP port %2d: %s\n", i,
		       (dsp_en & (1 << i)) ? "enabled" : "disabled");

	printf("\n  CLKREQ Mode     (0x%05x): 0x%04x\n", PT21_CLKREQ_MODE, clkreq_mode);
	for (i = 0; i < PT21_NUM_PCIE_CLKREQ; i++)
		printf("    CLKREQ%d mode: %u\n", i, (clkreq_mode >> (i * 2)) & 0x3);

	printf("\n  CLKREQ PinSel   (0x%05x): 0x%08x\n", PT21_CLKREQ_PINSEL, clkreq_pinsel);
	for (i = 0; i < PT21_NUM_PCIE_CLKREQ; i++)
		printf("    CLKREQ%d pin:  %u\n", i, (clkreq_pinsel >> (i * 4)) & 0xF);

	/* Lane reversal: one byte per DSP port, no access gate needed */
	printf("\n  Lane Reversal (DSP ports 0-%d):\n", PT21_NUM_PCIE_LANES - 1);
	for (i = 0; i < PT21_NUM_PCIE_LANES; i++) {
		uint32_t reg = PT21_LANE_REV_BASE + i * PT21_LANE_REV_STEP;

		val = pt21_read_byte(mmio, reg);
		printf("    DSP port %2d (0x%05x): 0x%02x  %s\n",
		       i, reg, val, (val & 0x01) ? "reversed" : "normal");
	}

	/* PCIe Gen1 TX swing per lane, no access gate */
	printf("\n  PCIe Gen1 TX Swing (lanes 0-%d):\n", PT21_NUM_PCIE_LANES - 1);
	for (i = 0; i < PT21_NUM_PCIE_LANES; i++) {
		uint32_t reg = PT21_GEN1_SWING_BASE + i * PT21_GEN1_SWING_STEP;

		val = pt21_read_byte(mmio, reg);
		printf("    Lane %2d (0x%05x): 0x%02x\n", i, reg, val);
	}

	/* Thermal throttle */
	val = pt21_read_byte(mmio, PT21_THERMAL_CTRL);
	printf("\n  Thermal Control (0x%05x): 0x%02x\n", PT21_THERMAL_CTRL, val);
	printf("    Throttle enable: %s\n", (val & 0x02) ? "yes" : "no");
	printf("    Gen1 swing applied: %s\n", (val & 0x04) ? "yes" : "no");
	printf("    SI config applied:  %s\n", (val & 0x80) ? "yes" : "no");
	val = pt21_read_byte(mmio, PT21_THERMAL_THRESH);
	printf("  Thermal Threshold (0x%05x): 0x%02x\n", PT21_THERMAL_THRESH, val);
}

static void dump_sata_settings(uint8_t *mmio)
{
	int i;
	uint8_t val;
	uint16_t sata_did;
	uint32_t cap;

	printf("\n--- SATA Settings ---\n");

	val = pt21_read_byte(mmio, PT21_SATA_CTRL_REG);
	printf("  SATA Controller (0x%05x): 0x%02x  SATA %s\n",
	       PT21_SATA_CTRL_REG, val, (val & 0x02) ? "enabled" : "disabled");

	/* Port enable and device ID are behind the access gate */
	pt21_write_byte(mmio, PT21_BANK_GATE, PT21_BANK_UNLOCK);
	val      = pt21_read_byte(mmio, PT21_SATA_PORT_EN1);
	sata_did = pt21_read_word(mmio, PT21_SATA_DID_REG);
	pt21_write_byte(mmio, PT21_BANK_GATE, PT21_BANK_LOCK);
	printf("  SATA Mode       (0x%05x): 0x%04x  %s\n",
	       PT21_SATA_DID_REG, sata_did,
	       (sata_did == PT21_SATA_DID_RAID) ? "RAID" : "AHCI");
	printf("  Port Enable 1   (0x%05x): 0x%02x", PT21_SATA_PORT_EN1, val);
	for (i = 0; i < PT21_NUM_SATA_PORTS; i++)
		printf("  P%d=%s", i, (val & (1 << i)) ? "on" : "off");
	printf("\n");

	val = pt21_read_byte(mmio, PT21_SATA_PORT_EN2);
	printf("  Port Enable 2   (0x%05x): 0x%02x", PT21_SATA_PORT_EN2, val);
	for (i = 0; i < PT21_NUM_SATA_PORTS; i++)
		printf("  P%d=%s", i, (val & (1 << i)) ? "on" : "off");
	printf("\n");

	cap = pt21_read_dword(mmio, PT21_SATA_CAP);
	printf("  AHCI CAP        (0x%05x): 0x%08x\n", PT21_SATA_CAP, cap);
	printf("    AggrLinkPmCap (bit26): %s\n", (cap & (1 << 26)) ? "yes" : "no");
	printf("    SscCap        (bit14): %s\n", (cap & (1 << 14)) ? "yes" : "no");
	printf("    PscCap        (bit13): %s\n", (cap & (1 << 13)) ? "yes" : "no");
	printf("    CccsCap       (bit 7): %s\n", (cap & (1 <<  7)) ? "yes" : "no");

	val = pt21_read_byte(mmio, PT21_SATA_DEVSLP);
	printf("  DevSleep        (0x%05x): 0x%02x", PT21_SATA_DEVSLP, val);
	for (i = 0; i < PT21_NUM_SATA_PORTS; i++)
		printf("  P%d=%s", i, (val & (1 << i)) ? "on" : "off");
	printf("\n");

	val = pt21_read_byte(mmio, PT21_AHCI_MSI_CAP);
	printf("  AHCI MSI Cap    (0x%05x): 0x%02x  MSI %s\n",
	       PT21_AHCI_MSI_CAP, val, (val == 0x50) ? "enabled" : "disabled");

	printf("  Hotplug per port:\n");
	for (i = 0; i < PT21_NUM_SATA_PORTS; i++) {
		val = pt21_read_byte(mmio, PT21_SATA_HOTPLUG0 + i);
		printf("    Port %d (0x%05x): 0x%02x  hotplug %s\n",
		       i, PT21_SATA_HOTPLUG0 + i, val,
		       (val & 0x08) ? "enabled" : "disabled");
	}

	printf("\n--- SATA PHY Tuning ---\n");
	for (i = 0; i < PT21_NUM_SATA_PORTS; i++) {
		printf("  Port %d:\n", i);
		printf("    Gen1/2 Swing (0x%05x): 0x%02x\n",
		       sata_gen12_swing[i], pt21_read_byte(mmio, sata_gen12_swing[i]));
		printf("    Gen3 Swing   (0x%05x): 0x%02x\n",
		       sata_gen3_swing[i], pt21_read_byte(mmio, sata_gen3_swing[i]));
		printf("    Gen1 Emph    (0x%05x): 0x%02x\n",
		       sata_gen1_emph[i], pt21_read_byte(mmio, sata_gen1_emph[i]));
		printf("    Gen2 Emph    (0x%05x): 0x%02x\n",
		       sata_gen2_emph[i], pt21_read_byte(mmio, sata_gen2_emph[i]));
		printf("    Gen3 Emph    (0x%05x): 0x%02x\n",
		       sata_gen3_emph[i], pt21_read_byte(mmio, sata_gen3_emph[i]));
		printf("    Speed Reg    (0x%05x): 0x%02x\n",
		       sata_speed_reg[i], pt21_read_byte(mmio, sata_speed_reg[i]));
	}
}

static void dump_gpio(uint64_t gpio_phys)
{
	const uint8_t *gpio_mmio;

	if (!gpio_phys) {
		printf("\n--- GPIO: BAR not assigned ---\n");
		return;
	}

	gpio_mmio = map_physical(gpio_phys, PT21_GPIO_MMIO_SIZE);
	if (!gpio_mmio) {
		printf("\n--- GPIO: failed to map 0x%08llx ---\n",
		       (unsigned long long)gpio_phys);
		return;
	}

	printf("\n--- GPIO (MMIO base 0x%08llx) ---\n",
	       (unsigned long long)gpio_phys);
	printf("  Direction (0x00): 0x%08x\n", read32(gpio_mmio + 0x0));
	printf("  Output    (0x08): 0x%08x\n", read32(gpio_mmio + 0x8));

	unmap_physical((void *)gpio_mmio, PT21_GPIO_MMIO_SIZE);
}

static struct pci_dev *find_pt21_xhci(struct pci_access *pacc)
{
	struct pci_dev *dev;

	for (dev = pacc->devices; dev; dev = dev->next) {
		pci_fill_info(dev, PCI_FILL_IDENT);
		if (dev->vendor_id == PCI_VENDOR_ID_AMD &&
		    dev->device_id >= PT21_XHCI_DID_MIN &&
		    dev->device_id <= PT21_XHCI_DID_MAX)
			return dev;
	}
	return NULL;
}

static struct pci_dev *find_pt21_dev(struct pci_access *pacc, uint16_t device)
{
	struct pci_dev *dev;
	struct pci_filter filter;

	pci_filter_init(NULL, &filter);
	filter.vendor = PCI_VENDOR_ID_AMD;
	filter.device = device;

	for (dev = pacc->devices; dev; dev = dev->next)
		if (pci_filter_match(&filter, dev))
			return dev;
	return NULL;
}

int print_promontory(struct pci_access *pacc)
{
	struct pci_dev *usp, *xhci;
	uint8_t *xhci_mmio;
	uint64_t xhci_phys, gpio_phys;
	uint8_t efuse;

	printf("\n============= Promontory 21 ==============\n");

	usp = find_pt21_dev(pacc, PT21_USP_DID);
	if (!usp) {
		printf("No Promontory 21 USP found (1022:%04x).\n", PT21_USP_DID);
		return -1;
	}

	printf("Found Promontory 21 USP  (%04x:%04x) at %02x:%02x.%u\n",
	       usp->vendor_id, usp->device_id,
	       usp->bus, usp->dev, usp->func);

	xhci = find_pt21_xhci(pacc);
	if (!xhci) {
		printf("No Promontory 21 xHCI endpoint found.\n");
		return -1;
	}

	printf("Found Promontory 21 xHCI (%04x:%04x) at %02x:%02x.%u\n",
	       xhci->vendor_id, xhci->device_id,
	       xhci->bus, xhci->dev, xhci->func);

	if (find_pt21_dev(pacc, PT21_SATA_DID))
		printf("Found Promontory 21 SATA\n");

	pci_fill_info(xhci, PCI_FILL_BASES);
	xhci_phys = xhci->base_addr[0] & ~(uint64_t)0xF;

	if (!xhci_phys || xhci_phys == (uint64_t)~0ULL) {
		printf("xHCI BAR0 not assigned, cannot read indirect registers.\n");
		return -1;
	}

	printf("\nxHCI MMIO base: 0x%08llx\n", (unsigned long long)xhci_phys);

	xhci_mmio = map_physical(xhci_phys, PT21_XHCI_MMIO_SIZE);
	if (!xhci_mmio) {
		printf("Failed to map xHCI MMIO.\n");
		return -1;
	}

	efuse = pt21_read_byte(xhci_mmio, PT21_EFUSE_REG);
	printf("\nSilicon Revision (eFUSE 0x%05x): 0x%02x (%s)\n",
	       PT21_EFUSE_REG, efuse, (efuse & 0x08) ? "A2" : "A0/A1");

	dump_pcie_config(xhci_mmio);
	dump_usb_settings(xhci_mmio);
	dump_usb3_phy(xhci_mmio);
	dump_usb2_phy(xhci_mmio);
	dump_sata_settings(xhci_mmio);

	unmap_physical((void *)xhci_mmio, PT21_XHCI_MMIO_SIZE);

	/* GPIO MMIO BAR at USP PCI config offset 0x40, 16-byte aligned */
	gpio_phys = (uint64_t)(pci_read_long(usp, 0x40) & 0xFFFFFFF0U);
	dump_gpio(gpio_phys);

	return 0;
}
