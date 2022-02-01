/* SPDX-License-Identifier: GPL-2.0-only */

#include <types.h>
#include <arch/bootblock.h>
#include <device/pci_ops.h>

/* For SB HT chain only, mmconf is not ready yet */
static void set_bsp_node_ch_t_ext_node_cfg_en(void)
{
	u32 dword;

	if (CONFIG(EXT_RT_TBL_SUPPORT)) {
		dword = pci_read_config32(PCI_DEV(0, 0x18, 0), 0x68);
		dword |= BIT(25) | BIT(27);
		/* CHtExtNodeCfgEn: coherent link extended node configuration enable,
		 * Nodes[31:0] will be 0xff:[31:0], Nodes[63:32] will be 0xfe:[31:0]
		 * ---- 32 nodes now only
		 * It can be used even nodes less than 8 nodes.
		 * We can have 8 more device on bus 0 in that case
		 */

		/* CHtExtAddrEn */
		pci_write_config32(PCI_DEV(0, 0x18, 0), 0x68, dword);
	}
}


void bootblock_early_northbridge_init(void) {
	/* Nothing special needs to be done to find bus 0 */
	/* Allow the HT devices to be found */
	/* mov bsp to bus 0xff when > 8 nodes */
	set_bsp_node_ch_t_ext_node_cfg_en();
}
