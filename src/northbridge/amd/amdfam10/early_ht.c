/*
 * This file is part of the coreboot project.
 *
 * Copyright (C) 2007 Advanced Micro Devices, Inc.
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
#include <stdint.h>
#include <types.h>
#include <device/pci_ops.h>
#include <device/pci_def.h>

#include "early_ht.h"

/* let't record the device of last ht device, so we can set the
 * Unit ID to CONFIG_HT_CHAIN_END_UNITID_BASE
 */
unsigned int real_last_unitid = 0;
uint8_t real_last_pos = 0;
uint32_t ht_dev_num = 0; // except host_bridge
uint8_t end_used = 0;

/* For SB HT chain only, mmconf is not ready yet */
void set_bsp_node_CHtExtNodeCfgEn(void)
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

static void set_ht_chain_unit_count(void)
{
	if (CONFIG_HT_CHAIN_END_UNITID_BASE != 0x20)
		return;

	if ((ht_dev_num > 1) && (unit_id != CONFIG_HT_CHAIN_END_UNITID_BASE) && !end_used) {
		uint16_t flags;
		flags = pci_read_config16(PCI_DEV(0, unit_id, 0), offset + PCI_CAP_FLAGS);
		flags &= ~0x1f;
		flags |= CONFIG_HT_CHAIN_END_UNITID_BASE & 0x1f;
		pci_write_config16(PCI_DEV(0, unit_id, 0), offset + PCI_CAP_FLAGS, flags);
	}
}

static int wait_for_ht_link_init_complete(pci_devfn_t devx, unsigned int offset)
{
	unsigned int ctrl;

	do {
		if (ctrl & (BIT(4) | BIT(8))) {
			/*
			 * Either the link has failed, or we have
			 * a CRC error.
			 * Sometimes this can happen due to link
			 * retrain, so lets knock it down and see
			 * if its transient
			 */
			ctrl |= (BIT(4) | BIT(8); // Link fail + Crc
			pci_write_config16(devx, offset, ctrl);
			ctrl = pci_read_config16(devx, offset);
			/* can not clear the error? */
			if (ctrl & (BIT(4) | BIT(8)))
				break;
		}
	} while ((ctrl & BIT(5)) == 0);

	return 0;
}

static int pci_valid(void)
{
	uint32_t id = pci_read_config32(PCI_DEV(0,0,0), PCI_VENDOR_ID);
	/* If the chain is enumerated quit */
	if (((id & 0xffff) == 0x0000) || ((id & 0xffff) == 0xffff) ||
		(((id >> 16) & 0xffff) == 0xffff) ||
		(((id >> 16) & 0xffff) == 0x0000))
	{
		return 1;
	}

	return 0;
}

static uint8_t get_pci_capabiliy_offset(void)
{
	uint8_t hdr_type = pci_read_config8(PCI_DEV(0,0,0), PCI_HEADER_TYPE) & 0x7f;

	if ((hdr_type == PCI_HEADER_TYPE_NORMAL) ||
		(hdr_type == PCI_HEADER_TYPE_BRIDGE)) {
		return pci_read_config8(PCI_DEV(0,0,0), PCI_CAPABILITY_LIST);
	}

	return 0;
}

static uint8_t is_pci_capability_ht(void)
{
	uint8_t pos = get_pci_capabiliy_offset();

	if (pos == 0)
		return 0;

	if (pci_read_config8(PCI_DEV(0,0,0), pos + PCI_CAP_LIST_ID) == PCI_CAP_ID_HT)
		return 1;

	return 0;
}

static void enumerate_ht_links(unsigned int *next_unitid)
{
	uint8_t pos = get_pci_capabiliy_offset();

	while (pos != 0) {
		int sts;

		if (is_pci_capability_ht()) {
			u16 flags;
			/* Read and write and reread flags so the link
				* direction bit is valid.
				*/
			flags = pci_read_config16(PCI_DEV(0,0,0), pos + PCI_CAP_FLAGS);
			pci_write_config16(PCI_DEV(0,0,0), pos + PCI_CAP_FLAGS, flags);
			flags = pci_read_config16(PCI_DEV(0,0,0), pos + PCI_CAP_FLAGS);
			/* is primary ? */
			if ((flags >> 13) == 0) {
				unsigned int count;
				unsigned int ctrl, ctrl_off;
				pci_devfn_t devx;

					if (*next_unitid >= 0x18) {
						if (!end_used) {
							*next_unitid = CONFIG_HT_CHAIN_END_UNITID_BASE;
							end_used = 1;
						} else {
							set_ht_chain_unit_count(real_last_unitid, real_last_pos);
						}
					}
					real_last_unitid = *next_unitid;
					real_last_pos = pos;
					ht_dev_num++;

				flags &= ~0x1f;
				flags |= *next_unitid & 0x1f;
				count = (flags >> 5) & 0x1f;
				devx = PCI_DEV(0, *next_unitid, 0);
				*next_unitid += count;

				pci_write_config16(PCI_DEV(0, 0, 0), pos + PCI_CAP_FLAGS, flags);

				/* Test for end of chain */
				ctrl_off = ((flags >> 10) & 1)?
					PCI_HT_CAP_SLAVE_CTRL0 : PCI_HT_CAP_SLAVE_CTRL1;

				/* Is this the end of the hypertransport chain? */
				if (pci_read_config16(devx, pos + ctrl_off) & BIT(6)) {
					set_ht_chain_unit_count(real_last_unitid, real_last_pos);
					return;
				}

				wait_for_ht_link_init_complete(devx, pos + ctrl_off);

				break;
			}
		}
		pos = pci_read_config8(PCI_DEV(0, 0, 0), pos + PCI_CAP_LIST_NEXT);
	}
}

void enumerate_ht_chain(void)
{
	/* Only one ht device in the ht chain, if so, don't need to go through the chain */
	if (CONFIG_HT_CHAIN_UNITID_BASE) == 0)
		return;

	/* Assumption the HT chain that is on bus 0 has the HT I/O Hub on it.
	 * On most boards this just happens. If a CPU has multiple non Coherent
	 * links the appropriate bus registers for the links needs to be
	 * programed to point at bus 0.
	 */
	unsigned int next_unitid, last_unitid = 0;

	next_unitid = CONFIG_HT_CHAIN_UNITID_BASE;
	do {
		u32 id;
		u8 hdr_type, pos;
		last_unitid = next_unitid;

		if (pci_valid())
			break;
		enumerate_ht_links(&next_unitid);

	} while (last_unitid != next_unitid);

}
