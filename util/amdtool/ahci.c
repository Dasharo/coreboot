/* ahci.c: dump AHCI registers */
/* SPDX-License-Identifier: GPL-2.0-only */

#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include "amdtool.h"
#include "smn.h"

static const char *ghc_regs[] = {
	"CAP", "GHC", "IS", "PI",
	"VS", "CCC_CTL", "CCC_PORTS", "EM_LOC",
	"EM_CTL", "CAP2", "BOHC"
};

static const char *port_ctl_regs[] = {
	"PxCLB", "PxCLBU", "PxFB", "PxFBU",
	"PxIS", "PxIE", "PxCMD", "Reserved",
	"PxTFD", "PxSIG", "PxSSTS", "PxSCTL",
	"PxSERR", "PxSACT", "PxCI", "PxSNTF",
	"PxFBS", "PxDEVSLP", "Reserved"
};

static const io_register_t ahci_cfg_registers[] = {
	{0x0, 4, "ID"},
	{0x4, 2, "CMD"},
	{0x6, 2, "STS"},
	{0x8, 1, "RID"},
	{0x9, 1, "PI"},
	{0xa, 2, "CC"},
	{0xc, 1, "CLS"},
	{0xd, 1, "MLT"},
	{0xe, 1, "HTYPE"},
	{0x10, 4, "MXTBA"},
	{0x14, 4, "MXPBA"},
	{0x20, 4, "AIDPBA"},
	{0x24, 4, "ABAR"},
	{0x2c, 4, "SS"},
	{0x30, 4, "EROM"},
	{0x34, 1, "CAP"},
	{0x3c, 2, "INTR"},
	{0x3e, 1, "MGNT"},
	{0x3f, 1, "MLAT"},
};

#define NUM_GHC (sizeof(ghc_regs)/sizeof(ghc_regs[0]))
#define NUM_PORTCTL (sizeof(port_ctl_regs)/sizeof(port_ctl_regs[0]))

static void print_port(const uint8_t *const mmio, size_t port)
{
	size_t i;
	printf("\nPort %zu Control Registers:\n", port);
	const uint8_t *const mmio_port = mmio + 0x100 + port * 0x80;
	for (i = 0; i < 0x80; i += 4) {
		if (i / 4 < NUM_PORTCTL) {
			printf("0x%03zx: 0x%08x (%s)\n",
			       (size_t)(mmio_port - mmio) + i,
			       read32(mmio_port + i), port_ctl_regs[i / 4]);
		} else if (read32(mmio_port + i)) {
			printf("0x%03zx: 0x%08x (Reserved)\n",
			       (size_t)(mmio_port - mmio) + i,
			       read32(mmio_port + i));
		}
	}
}

static int print_ahci(struct pci_dev *ahci)
{
	size_t ahci_registers_size = 0x800, i;
	size_t ahci_cfg_registers_size = ARRAY_SIZE(ahci_cfg_registers);

	if (!ahci)
		return 0;

	printf("\n============= AHCI (%02x:%02x.%u) ==============\n",
	       ahci->bus, ahci->dev, ahci->func);

	printf("\n=========== Configuration Registers ============\n\n");
	for (i = 0; i < ahci_cfg_registers_size; i++) {
		switch (ahci_cfg_registers[i].size) {
		case 4:
			printf("0x%04x: 0x%08x (%s)\n",
				ahci_cfg_registers[i].addr,
				pci_read_long(ahci, ahci_cfg_registers[i].addr),
				ahci_cfg_registers[i].name);
			break;
		case 2:
			printf("0x%04x: 0x%04x     (%s)\n",
				ahci_cfg_registers[i].addr,
				pci_read_word(ahci, ahci_cfg_registers[i].addr),
				ahci_cfg_registers[i].name);
			break;
		case 1:
			printf("0x%04x: 0x%02x       (%s)\n",
				ahci_cfg_registers[i].addr,
				pci_read_byte(ahci, ahci_cfg_registers[i].addr),
				ahci_cfg_registers[i].name);
			break;
		}
	}

	pci_fill_info(ahci, PCI_FILL_BASES);

	if (ahci->base_addr[5] == 0ULL || ahci->base_addr[5] == UINT64_MAX) {
		perror("ABAR not assigned\n");
		return -1;
	}

	const pciaddr_t ahci_phys = ahci->base_addr[5] & ~0x7ULL;
	printf("\n============= ABAR ==============\n\n");
	printf("ABAR = 0x%08llx (MEM)\n\n", (unsigned long long)ahci_phys);
	const uint8_t *const mmio = map_physical(ahci_phys, ahci_registers_size);
	if (mmio == NULL) {
		perror("Error mapping MMIO\n");
		exit(1);
	}

	puts("Generic Host Control Registers:");
	for (i = 0; i < 0x100; i += 4) {
		if (i / 4 < NUM_GHC) {
			printf("0x%03zx: 0x%08x (%s)\n",
			       i, read32(mmio + i), ghc_regs[i / 4]);
		} else if (read32(mmio + i)) {
			printf("0x%03zx: 0x%08x (Reserved)\n", i,
			       read32(mmio + i));
		}
	}

	const size_t max_ports = (read32(mmio) & 0x1f) + 1;
	for (i = 0; i < max_ports; i++) {
		if (read32(mmio + 0x0c) & 1 << i)
			print_port(mmio, i);
	}

	puts("\nOther registers:");
	for (i = 0x500; i < ahci_registers_size; i += 4) {
		if (read32(mmio + i))
			printf("0x%03zx: 0x%08x\n", i, read32(mmio + i));
	}

	unmap_physical((void *)mmio, ahci_registers_size);
	return 0;
}

#define FCH_KL_SMN_SATA_CONTROL_BASE         0x03100000ul
#define FCH_KL_SMN_SATA_CONTROL_RSMU         FCH_KL_SMN_SATA_CONTROL_BASE
#define FCH_KL_SMN_SATA_CONTROL_SLOR         (FCH_KL_SMN_SATA_CONTROL_BASE + 0x1800)
#define FCH_KL_SMN_SATA_STEP                 0x100000ul

static void print_ahci_smn(struct pci_dev *nb)
{
	static uint32_t ctrlr = 0;
	size_t i;
	uint32_t smn_base = 0;
	uint32_t sata_rsmu_smn_base = 0;
	uint32_t sata_slor_smn_base = 0;
	uint32_t step = 0;
	uint32_t slor_size = 0;

	if (!nb || nb->vendor_id != PCI_VENDOR_ID_AMD)
		return;

	switch(nb->device_id) {
	case PCI_DEVICE_ID_AMD_BRH_ROOT_COMPLEX:
		sata_rsmu_smn_base = FCH_KL_SMN_SATA_CONTROL_RSMU;
		sata_slor_smn_base = FCH_KL_SMN_SATA_CONTROL_SLOR;
		step = FCH_KL_SMN_SATA_STEP;
		slor_size = 0x400;
	default:
		return;
	}

	smn_base = sata_slor_smn_base + ctrlr * step;

	puts("\nSLOR:");
	for (i = 0; i < slor_size; i += 4) {
		if (smn_read32(smn_base + i))
			printf("0x%03zx: 0x%08x\n", i, smn_read32(smn_base + i));
	}

	smn_base = sata_rsmu_smn_base + ctrlr * step;

	printf("SATA_MISC_CONTROL: %04x\n", smn_read16(smn_base + 0x00));
	printf("SATA_OOB_CONTROL: %04x\n", smn_read16(smn_base + 0x02));
	printf("SATA_AOAC_CONTROL: %08x\n", smn_read32(smn_base + 0x04));
	printf("SATA_EVENT_SELECT: %04x\n", smn_read16(smn_base + 0x08));
	printf("SATA_NBIF_CONTROL: %04x\n", smn_read16(smn_base + 0x0A));

	ctrlr++;
}

int print_ahci_devs(struct pci_access *pacc, struct pci_dev *nb)
{
	struct pci_dev *dev;
	struct pci_filter filter;

	printf("\n============= AHCI ==============\n\n");

	pci_filter_init(NULL, &filter);
	filter.vendor = PCI_VENDOR_ID_AMD;
	filter.device = PCI_DEVICE_ID_AMD_FCH_SATA_AHCI_2;

	for (dev = pacc->devices; dev; dev = dev->next) {
		if (pci_filter_match(&filter, dev)) {
			print_ahci(dev);
			print_ahci_smn(nb);
		}
	}

	filter.vendor = PCI_VENDOR_ID_AMD;
	filter.device = PCI_DEVICE_ID_AMD_FCH_SATA_AHCI_1;

	for (dev = pacc->devices; dev; dev = dev->next)
		if (pci_filter_match(&filter, dev))
			print_ahci(dev);

	return 0;
}
