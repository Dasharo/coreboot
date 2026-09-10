/* SPDX-License-Identifier: GPL-2.0-or-later */

#ifndef __DRIVERS_ASPEED_COMMON_AST_AHB_H__
#define __DRIVERS_ASPEED_COMMON_AST_AHB_H__

#include <stddef.h>
#include <stdint.h>
#include <types.h>

/* P2A bridge control registers, offsets into PCI BAR1 (register MMIO space) */
#define AST_P2A_BRIDGE_CTRL	0xf000
#define AST_P2A_BRIDGE_ADDR	0xf004

/*
 * Write `len` bytes to the BMC's internal AHB address space (SCU, SDRAM,
 * ...) through the P2A (PCI-to-AHB) bridge exposed by the ASPEED BMC's PCI
 * VGA/graphics function.
 *
 * This gives the host CPU a way to reach BMC memory that the BMC itself can
 * also map (e.g. the "VGA shared memory" window OpenBMC's smbios-mdr/
 * intel-ipmi-oem daemons map via /dev/vgasharedmem), without requiring any
 * cooperation from the BMC beyond having the P2A bridge enabled, which is
 * the default on AST2500/AST2600 unless locked down by BMC firmware.
 *
 * `bmc_addr` and `len` may be arbitrary; unaligned leading/trailing bytes
 * are handled with a read-modify-write of the containing dword.
 *
 * Returns:
 * - CB_SUCCESS on success.
 * - CB_ERR if the bridge could not be reached (BMC graphics device not found,
 *   P2A bridge disabled/timed out, etc.)
 */
enum cb_err ast_ahb_write_buffer(uint32_t bmc_addr, const void *buf, size_t len);

#endif /* __DRIVERS_ASPEED_COMMON_AST_AHB_H__ */
