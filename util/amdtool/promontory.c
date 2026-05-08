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
 * Write 3-byte address (MSB first), then read data byte.
 * Status bit 7 = busy; poll until clear after each address byte write.
 */
#define PT21_IND_ADDR2       0x3002
#define PT21_IND_ADDR1       0x3001
#define PT21_IND_ADDR0       0x3000
#define PT21_IND_DATA        0x3008
#define PT21_IND_STATUS      0x3009
#define PT21_IND_BUSY        0x80

/* Promontory 21 internal (indirect) register addresses */
#define PT21_PME_REG         0x18515
#define PT21_DBC_REG         0x18A61
#define PT21_EFUSE_REG       0x2E37B
#define PT21_SATA_CTRL_REG   0x10151
#define PT21_SATA_PORT_EN1   0x2471C
#define PT21_SATA_PORT_EN2   0x2E006
#define PT21_SATA_CLKREQ     0x24720

#define PT21_NUM_USB3_PORTS        6
#define PT21_NUM_USB2_PORT_GROUPS  3	/* 2 ports per group */
#define PT21_NUM_HW_LPM            3
#define PT21_NUM_SATA_PORTS        4

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

static void pt21_wait_ready(uint8_t *mmio)
{
	unsigned int retries = 10000;

	while ((read8(mmio + PT21_IND_STATUS) & PT21_IND_BUSY) && retries--)
		;
}

/*
 * Read one byte from a Promontory 21 internal register via the indirect
 * access mechanism at xHCI MMIO offsets 0x3000-0x3009.
 * Matches Prom21XhciReadByte() in PromAccess.c.
 */
static uint8_t pt21_read_byte(uint8_t *mmio, uint32_t addr)
{
	/* Dummy status read (matches firmware behaviour) */
	(void)read8(mmio + PT21_IND_STATUS);

	write8(mmio + PT21_IND_ADDR2, (addr >> 16) & 0xFF);
	pt21_wait_ready(mmio);

	write8(mmio + PT21_IND_ADDR1, (addr >> 8) & 0xFF);
	pt21_wait_ready(mmio);

	write8(mmio + PT21_IND_ADDR0, addr & 0xFF);
	pt21_wait_ready(mmio);

	return read8(mmio + PT21_IND_DATA);
}

static void dump_pci_cfg(struct pci_dev *dev, const char *label)
{
	int i;
	uint32_t val;

	printf("\n--- PCI Config: %s (%04x:%04x @ %02x:%02x.%u) ---\n",
	       label, dev->vendor_id, dev->device_id,
	       dev->bus, dev->dev, dev->func);

	for (i = 0; i < 0x100; i += 4) {
		val = pci_read_long(dev, i);
		if (val != 0xFFFFFFFF)
			printf("  0x%03x: 0x%08x\n", i, val);
	}
}

static void dump_usb_settings(uint8_t *mmio)
{
	int i;
	uint8_t val;

	printf("\n--- USB Settings ---\n");

	val = pt21_read_byte(mmio, PT21_PME_REG);
	printf("  PME Control (0x%05x): 0x%02x  PME %s\n",
	       PT21_PME_REG, val, (val & 0x80) ? "disabled" : "enabled");

	val = pt21_read_byte(mmio, PT21_DBC_REG);
	printf("  DbC Control (0x%05x): 0x%02x\n", PT21_DBC_REG, val);

	printf("  HW LPM:\n");
	for (i = 0; i < PT21_NUM_HW_LPM; i++) {
		val = pt21_read_byte(mmio, hw_lpm_en[i]);
		printf("    Group %d (0x%05x): 0x%02x  bits[4:1]=0x%x\n",
		       i, hw_lpm_en[i], val, (val >> 1) & 0xF);
	}
}

static void dump_usb3_phy(uint8_t *mmio)
{
	int i;

	printf("\n--- USB3 PHY Tuning ---\n");
	for (i = 0; i < PT21_NUM_USB3_PORTS; i++) {
		printf("  Port %d:\n", i);
		printf("    Gen1 Swing          (0x%05x): 0x%02x\n",
		       usb3_gen1_swing[i], pt21_read_byte(mmio, usb3_gen1_swing[i]));
		printf("    Gen1 Emph/Preshoot  (0x%05x): 0x%02x\n",
		       usb3_gen1_ep[i], pt21_read_byte(mmio, usb3_gen1_ep[i]));
		printf("    Gen2 Swing          (0x%05x): 0x%02x\n",
		       usb3_gen2_swing[i], pt21_read_byte(mmio, usb3_gen2_swing[i]));
		printf("    Gen2 CP0  Emph/Pre  (0x%05x): 0x%02x\n",
		       usb3_gen2_cp0_ep[i], pt21_read_byte(mmio, usb3_gen2_cp0_ep[i]));
		printf("    Gen2 CP13 Emph/Pre  (0x%05x): 0x%02x\n",
		       usb3_gen2_cp13_ep[i], pt21_read_byte(mmio, usb3_gen2_cp13_ep[i]));
		printf("    Gen2 CP14 Emph/Pre  (0x%05x): 0x%02x\n",
		       usb3_gen2_cp14_ep[i], pt21_read_byte(mmio, usb3_gen2_cp14_ep[i]));
		printf("    Gen2 CP15 Emph/Pre  (0x%05x): 0x%02x\n",
		       usb3_gen2_cp15_ep[i], pt21_read_byte(mmio, usb3_gen2_cp15_ep[i]));
		printf("    Gen2 CP16 Emph/Pre  (0x%05x): 0x%02x\n",
		       usb3_gen2_cp16_ep[i], pt21_read_byte(mmio, usb3_gen2_cp16_ep[i]));
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

static void dump_sata_settings(uint8_t *mmio)
{
	int i;
	uint8_t val;

	printf("\n--- SATA Settings ---\n");

	val = pt21_read_byte(mmio, PT21_SATA_CTRL_REG);
	printf("  SATA Controller (0x%05x): 0x%02x  SATA %s\n",
	       PT21_SATA_CTRL_REG, val, (val & 0x02) ? "disabled" : "enabled");

	val = pt21_read_byte(mmio, PT21_SATA_PORT_EN1);
	printf("  Port Enable 1   (0x%05x): 0x%02x\n", PT21_SATA_PORT_EN1, val);

	val = pt21_read_byte(mmio, PT21_SATA_PORT_EN2);
	printf("  Port Enable 2   (0x%05x): 0x%02x\n", PT21_SATA_PORT_EN2, val);

	val = pt21_read_byte(mmio, PT21_SATA_CLKREQ);
	printf("  CLKREQ          (0x%05x): 0x%02x\n", PT21_SATA_CLKREQ, val);

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
	struct pci_dev *usp, *xhci, *sata;
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

	sata = find_pt21_dev(pacc, PT21_SATA_DID);
	if (sata)
		printf("Found Promontory 21 SATA (%04x:%04x) at %02x:%02x.%u\n",
		       sata->vendor_id, sata->device_id,
		       sata->bus, sata->dev, sata->func);

	dump_pci_cfg(usp, "Promontory21 USP");
	if (sata)
		dump_pci_cfg(sata, "Promontory21 SATA");
	dump_pci_cfg(xhci, "Promontory21 xHCI");

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
