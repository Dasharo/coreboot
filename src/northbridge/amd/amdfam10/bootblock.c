/*
 * This file is part of the coreboot project.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <types.h>
#include <bootblock_common.h>
#include "northbridge/amd/amdfam10/early_ht.c"

// For SB HT chain only
// mmconf is not ready yet
void set_bsp_node_CHtExtNodeCfgEn(void)
{
	u32 reg;

	if (CONFIG(EXT_RT_TBL_SUPPORT)) {
		reg = pci_io_read_config32(PCI_DEV(0, 0x18, 0), 0x68);
		reg |= BIT(27) | BIT(25);
		/* CHtExtNodeCfgEn: coherent link extended node configuration enable,
		 * Nodes[31:0] will be 0xff:[31:0], Nodes[63:32] will be 0xfe:[31:0]
		 * ---- 32 nodes now only
		 * It can be used even nodes less than 8 nodes.
		 * We can have 8 more device on bus 0 in that case
		 */

		/* CHtExtAddrEn */
		pci_io_write_config32(PCI_DEV(0, 0x18, 0), 0x68, reg);
	}
}

void bootblock_early_northbridge_init(void) {
	/* Nothing special needs to be done to find bus 0 */
	/* Allow the HT devices to be found */
	/* mov bsp to bus 0xff when > 8 nodes */
	set_bsp_node_CHtExtNodeCfgEn();
	enumerate_ht_chain();
}
