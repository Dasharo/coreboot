/* SPDX-License-Identifier: GPL-2.0-only */

#include <commonlib/region.h>
#include <console/console.h>
#include <dasharo/options.h>
#include <fmap.h>
#include <intelblocks/cse.h>
#include <types.h>
#include <variant/pcie.h>

#define SI_DESC_SIZE		0x1000
#define FLMAP1_OFFSET		0x18

/*
 * Lane configuration of PCIe controller 3 (RP9-RP12, the M.2 M-key slot) is in
 * PCHSTRP28[12:11], i.e. bits 4:3 of the strap's second byte. Values taken from
 * descriptors generated with mFIT (Flex IO, "PCIe Controller 3"), which differ
 * only in this field. The other bits of the byte are left as they are.
 */
#define PCHSTRP28		28
#define PCIE_CTRL3_BYTE		1
#define PCIE_CTRL3_MASK		0x18
#define PCIE_CTRL3_1X4		0x18
#define PCIE_CTRL3_2X2		0x10
#define PCIE_CTRL3_1X2_2X1	0x08
#define PCIE_CTRL3_4X1		0x00

static size_t pcie_ctrl3_offset(uint32_t flmap1)
{
	/* FLMAP1[23:16] is FPSBA, the base of the PCH straps in 16-byte units */
	return (((flmap1 >> 16) & 0xff) << 4) + PCHSTRP28 * sizeof(uint32_t) +
	       PCIE_CTRL3_BYTE;
}

bool variant_pcie_is_2x2(void)
{
	struct region_device rdev;
	uint32_t flmap1;
	uint8_t strap;

	/*
	 * The descriptor is outside the BIOS region, which is all the read-only
	 * boot device maps here (no FAST_SPI_FULL_RO_BOOTMEDIA), so read it
	 * through the SPI controller.
	 */
	if (fmap_locate_area_as_rdev_rw("SI_DESC", &rdev) < 0 ||
	    rdev_readat(&rdev, &flmap1, FLMAP1_OFFSET, sizeof(flmap1)) != sizeof(flmap1) ||
	    rdev_readat(&rdev, &strap, pcie_ctrl3_offset(flmap1), sizeof(strap)) != sizeof(strap)) {
		printk(BIOS_ERR, "PCIe bifurcation: failed to read the descriptor\n");
		return false;
	}

	printk(BIOS_DEBUG, "PCIe bifurcation: controller 3 strap 0x%02x (%s)\n", strap,
	       (strap & PCIE_CTRL3_MASK) == PCIE_CTRL3_2X2 ? "2x2" : "not 2x2");

	return (strap & PCIE_CTRL3_MASK) == PCIE_CTRL3_2X2;
}

void mainboard_update_descriptor(uint8_t *si_desc)
{
	const size_t offset = pcie_ctrl3_offset(*(uint32_t *)(si_desc + FLMAP1_OFFSET));
	const uint8_t option = dasharo_get_pcie_bifurcation();
	uint8_t value, strap;

	if (offset >= SI_DESC_SIZE) {
		printk(BIOS_ERR, "PCIe bifurcation: PCH strap base out of range\n");
		return;
	}

	switch (option) {
	case PCIE_BIFURCATION_OPTION_1X4:
		value = PCIE_CTRL3_1X4;
		break;
	case PCIE_BIFURCATION_OPTION_2X2:
		value = PCIE_CTRL3_2X2;
		break;
	default:
		printk(BIOS_WARNING, "PCIe bifurcation option %u is not supported, using 1x4\n",
		       option);
		value = PCIE_CTRL3_1X4;
		break;
	}

	strap = (si_desc[offset] & ~PCIE_CTRL3_MASK) | value;
	if (si_desc[offset] != strap) {
		printk(BIOS_INFO, "PCIe bifurcation: descriptor byte 0x%zx 0x%02x -> 0x%02x\n",
		       offset, si_desc[offset], strap);
		si_desc[offset] = strap;
	}
}
