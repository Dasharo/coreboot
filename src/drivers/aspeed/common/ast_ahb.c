/* SPDX-License-Identifier: GPL-2.0-or-later */

/*
 * Access to the ASPEED BMC's internal AHB address space through the P2A
 * (PCI-to-AHB) bridge exposed by its PCI VGA/graphics function.
 *
 * The bridge is controlled through two 32-bit registers in the graphics
 * device's register BAR (BAR1): a window base address register and a
 * "go" control register. Once a 64 KiB aligned window has been selected,
 * the corresponding 64 KiB of BMC AHB address space becomes visible at a
 * fixed offset in the same BAR. This is the same mechanism used internally
 * by the AST display driver (see ast_mindwm()/ast_moutdwm() in
 * ast_post.c) to reach SCU and other BMC-internal registers; this file
 * generalizes it into a byte-buffer oriented API for other drivers to use.
 */

#include <commonlib/helpers.h>
#include <console/console.h>
#include <device/device.h>
#include <device/mmio.h>
#include <device/pci.h>
#include <device/pci_def.h>
#include <device/pci_ids.h>
#include <device/resource.h>
#include <string.h>
#include <types.h>

#include "ast_ahb.h"

/* Once configured, the selected 64 KiB AHB window appears at this offset */
#define AST_P2A_WINDOW_BASE	0x10000
#define AST_P2A_WINDOW_SIZE	0x10000
#define AST_P2A_WINDOW_MASK	(AST_P2A_WINDOW_SIZE - 1)

#define AST_P2A_TIMEOUT_LOOPS	100000

static void *ast_ahb_regs(void)
{
	static void *regs;
	static bool probed;
	struct device *dev;
	struct resource *res;

	if (probed)
		return regs;

	probed = true;

	dev = dev_find_device(PCI_VID_ASPEED, PCI_DID_ASPEED_AST2050_VGA, NULL);
	if (!dev) {
		printk(BIOS_ERR, "AST AHB: BMC graphics device not found\n");
		return NULL;
	}

	res = probe_resource(dev, PCI_BASE_ADDRESS_1);
	if (!res) {
		printk(BIOS_ERR, "AST AHB: BAR1 resource not found\n");
		return NULL;
	}

	regs = res2mmio(res, 0, 0);

	return regs;
}

static enum cb_err ast_ahb_set_window(void *regs, uint32_t addr)
{
	unsigned int timeout;
	uint32_t base = addr & ~AST_P2A_WINDOW_MASK;

	write32(regs + AST_P2A_BRIDGE_ADDR, base);
	write32(regs + AST_P2A_BRIDGE_CTRL, 0x1);

	for (timeout = 0; timeout < AST_P2A_TIMEOUT_LOOPS; timeout++) {
		if (read32(regs + AST_P2A_BRIDGE_ADDR) == base)
			return CB_SUCCESS;
	}

	printk(BIOS_ERR, "AST AHB: timeout configuring P2A window for 0x%08x\n", addr);
	return CB_ERR;
}

enum cb_err ast_ahb_write_buffer(uint32_t bmc_addr, const void *buf, size_t len)
{
	void *regs = ast_ahb_regs();
	const uint8_t *src = buf;

	if (!regs)
		return CB_ERR;

	if (bmc_addr % sizeof(uint32_t)) {
		printk(BIOS_ERR, "AST AHB: unaligned address 0x%08x\n", bmc_addr);
		return CB_ERR;
	}

	while (len) {
		uint32_t window_off = bmc_addr & AST_P2A_WINDOW_MASK;
		size_t window_remaining = AST_P2A_WINDOW_SIZE - window_off;

		if (ast_ahb_set_window(regs, bmc_addr) != CB_SUCCESS)
			return CB_ERR;

		while (len && window_remaining) {
			uint32_t val;
			size_t n = MIN(len, sizeof(val));

			if (n == sizeof(val)) {
				memcpy(&val, src, sizeof(val));
			} else {
				/* Trailing partial dword: read-modify-write so we
				   only touch the bytes actually requested. */
				val = read32(regs + AST_P2A_WINDOW_BASE + window_off);
				memcpy(&val, src, n);
			}
			write32(regs + AST_P2A_WINDOW_BASE + window_off, val);

			src += n;
			window_off += sizeof(val);
			window_remaining -= sizeof(val);
			bmc_addr += sizeof(val);
			len -= n;
		}
	}

	return CB_SUCCESS;
}
