/* SPDX-License-Identifier: GPL-2.0-only */

#include <stdint.h>
#include <arch/cpu.h>
#include <console/console.h>
#include <cpu/amd/common/common.h>
#include <cpu/amd/common/nums.h>
#include <cpu/amd/amddefs.h>
#include <cpu/amd/msr.h>
#include <cpu/amd/mtrr.h>
#include <cpu/x86/mtrr.h>
#include <cpu/x86/lapic.h>
#include <device/pci_ops.h>
#include <drivers/amd/hypertransport/ht_wrapper.h>
#include <northbridge/amd/amdfam10/amdfam10.h>
#include <southbridge/amd/sb700/sb700.h>
#include <option.h>

#include "defaults.h"

static void AMD_Errata281(u8 node, u64 revision, u32 platform)
{
	/* Workaround for Transaction Scheduling Conflict in
	 * Northbridge Cross Bar. Implement XCS Token adjustment
	 * for ganged links. Also, perform fix up for the mixed
	 * revision case.
	 */

	u32 reg, val;
	u8 i;
	u8 mixed = 0;
	u8 nodes = get_nodes();

	if (!(platform & AMD_PTYPE_SVR)) {
		return;
	}
	/* For each node we need to check for a "broken" node */
	if (!(revision & (AMD_DR_B0 | AMD_DR_B1))) {
		for (i = 0; i < nodes; i++) {
			if (get_logical_CPUID(i) & (AMD_DR_B0 | AMD_DR_B1)) {
				mixed = 1;
				break;
			}
		}
	}

	if ((revision & (AMD_DR_B0 | AMD_DR_B1)) || mixed) {
		/* F0X68[22:21] DsNpReqLmt0 = 01b */
		val = pci_read_config32(NODE_PCI(node, 0), 0x68);
		val &= ~0x00600000;
		val |= 0x00200000;
		pci_write_config32(NODE_PCI(node, 0), 0x68, val);

		/* F3X6C */
		val = pci_read_config32(NODE_PCI(node, 3), 0x6C);
		val &= ~0x700780F7;
		val |= 0x00010094;
		pci_write_config32(NODE_PCI(node, 3), 0x6C, val);

		/* F3X7C */
		val = pci_read_config32(NODE_PCI(node, 3), 0x7C);
		val &= ~0x707FFF1F;
		val |= 0x00144514;
		pci_write_config32(NODE_PCI(node, 3), 0x7C, val);

		/* F3X144[3:0] RspTok = 0001b */
		val = pci_read_config32(NODE_PCI(node, 3), 0x144);
		val &= ~0x0000000F;
		val |= 0x00000001;
		pci_write_config32(NODE_PCI(node, 3), 0x144, val);

		for (i = 0; i < 3; i++) {
			reg = 0x148 + (i * 4);
			val = pci_read_config32(NODE_PCI(node, 3), reg);
			val &= ~0x000000FF;
			val |= 0x000000DB;
			pci_write_config32(NODE_PCI(node, 3), reg, val);
		}
	}
}

/**
 * amd_set_ht_phy_register - Use the HT link's HT Phy portal registers to update
 * a phy setting for that link.
 */
static void amd_set_ht_phy_register(u8 node, u8 link, u8 entry)
{
	u32 phyReg;
	u32 phyBase;
	u32 val;

	/* Determine this link's portal */
	if (link > 3)
		link -= 4;

	phyBase = ((u32) link << 3) | 0x180;

	/* Determine if link is connected and abort if not */
	if (!(pci_read_config32(NODE_PCI(node, 0), 0x98 + (link * 0x20)) & 0x1))
		return;

	/* Get the portal control register's initial value
	 * and update it to access the desired phy register
	 */
	phyReg = pci_read_config32(NODE_PCI(node, 4), phyBase);

	if (fam10_htphy_default[entry].htreg > 0x1FF) {
		phyReg &= ~HTPHY_DIRECT_OFFSET_MASK;
		phyReg |= HTPHY_DIRECT_MAP;
	} else {
		phyReg &= ~HTPHY_OFFSET_MASK;
	}

	/* Now get the current phy register data
	 * LinkPhyDone = 0, LinkPhyWrite = 0 is a read
	 */
	phyReg |= fam10_htphy_default[entry].htreg;
	pci_write_config32(NODE_PCI(node, 4), phyBase, phyReg);

	do {
		val = pci_read_config32(NODE_PCI(node, 4), phyBase);
	} while (!(val & HTPHY_IS_COMPLETE_MASK));

	/* Now we have the phy register data, apply the change */
	val = pci_read_config32(NODE_PCI(node, 4), phyBase + 4);
	val &= ~fam10_htphy_default[entry].mask;
	val |= fam10_htphy_default[entry].data;
	pci_write_config32(NODE_PCI(node, 4), phyBase + 4, val);

	/* write it through the portal to the phy
	 * LinkPhyDone = 0, LinkPhyWrite = 1 is a write
	 */
	phyReg |= HTPHY_WRITE_CMD;
	pci_write_config32(NODE_PCI(node, 4), phyBase, phyReg);

	do {
		val = pci_read_config32(NODE_PCI(node, 4), phyBase);
	} while (!(val & HTPHY_IS_COMPLETE_MASK));
}

static void set_ht_phy_defaults(u8 node)
{
	u8 i;
	u8 j;
	u8 offset;
	u64 revision = get_logical_CPUID(node);
	u32 platform = get_platform_type();

	for (i = 0; i < ARRAY_SIZE(fam10_htphy_default); i++) {
		if ((fam10_htphy_default[i].revision & revision)
			&& (fam10_htphy_default[i].platform & platform)) {
			/* HT Phy settings either apply to both sublinks or have
			 * separate registers for sublink zero and one, so there
			 * will be two table entries. So, here we only loop
			 * through the sublink zeros in function zero.
			 */
			for (j = 0; j < 4; j++) {
				if (amd_cpu_find_capability(node, j, &offset)) {
					if (AMD_checkLinkType(node, offset)
						& fam10_htphy_default[i].linktype) {
						amd_set_ht_phy_register(node, j, i);
					}
				} else {
					/* No more capabilities,
					 * link not present
					 */
					break;
				}
			}
		}
	}
}

static void set_pci_node_reg(pci_devfn_t dev, u16 reg, u32 and_mask, u32 or_mask)
{
	u32 dword;

	dword = pci_read_config32(dev, reg);
	dword &= and_mask;
	dword |= or_mask;
	pci_write_config32(dev, reg, dword);
}

static void configure_ht_stop_tristate(u8 node)
{
	if (is_fam15h()) {
		if (CONFIG_CPU_SOCKET_TYPE == 0x14) {
			/* Socket C32 LdtStopTriEn = 1 */
			set_pci_node_reg(NODE_PCI(node, 0), 0x84, ~BIT(13), BIT(13));
			set_pci_node_reg(NODE_PCI(node, 0), 0xa4, ~BIT(13), BIT(13));
			set_pci_node_reg(NODE_PCI(node, 0), 0xc4, ~BIT(13), BIT(13));
			set_pci_node_reg(NODE_PCI(node, 0), 0xe4, ~BIT(13), BIT(13));
		}
		else {
			/* Other socket (G34, etc.) LdtStopTriEn = 0 */
			set_pci_node_reg(NODE_PCI(node, 0), 0x84, ~BIT(13), 0);
			set_pci_node_reg(NODE_PCI(node, 0), 0xa4, ~BIT(13), 0);
			set_pci_node_reg(NODE_PCI(node, 0), 0xc4, ~BIT(13), 0);
			set_pci_node_reg(NODE_PCI(node, 0), 0xe4, ~BIT(13), 0);
		}
	}
}


static void configure_pci_defaults(u8 node)
{
	u32 val;
	u64 revision = get_logical_CPUID(node);
	u32 platform = get_platform_type();

	for (u8 i = 0; i < ARRAY_SIZE(fam10_pci_default); i++) {
		if ((fam10_pci_default[i].revision & revision)
				&& (fam10_pci_default[i].platform & platform)) {
			val = pci_read_config32(NODE_PCI(node, fam10_pci_default[i].function),
						fam10_pci_default[i].offset);
			val &= ~fam10_pci_default[i].mask;
			val |= fam10_pci_default[i].data;
			pci_write_config32(NODE_PCI(node, fam10_pci_default[i].function),
						fam10_pci_default[i].offset, val);
		}
	}
}

static void configure_message_triggered_c1e(u8 node)
{
	u64 revision = get_logical_CPUID(node);

	if (revision & (AMD_DR_GT_D0 | AMD_FAM15_ALL)) {
		/* Set up message triggered C1E */
		/* CacheFlushImmOnAllHalt = !is_fam15h() */
		set_pci_node_reg(NODE_PCI(node, 3), 0xd4, ~BIT(14), is_fam15h() ? 0 : BIT(14));

		/* IgnCpuPrbEn = 1 */
		/* CacheFlushOnHaltTmr = 0x28 */
		/* CacheFlushOnHaltCtl = 0x7 */
		set_pci_node_reg(NODE_PCI(node, 3), 0xdc, ~(0x7f << 19), 0x05470000);
		/* IdleExitEn = 1 */
		set_pci_node_reg(NODE_PCI(node, 3), 0xa0, ~BIT(10), BIT(10));

		if (revision & AMD_DR_GT_D0) {
			/* EnStpGntOnFlushMaskWakeup = 1 */
			set_pci_node_reg(NODE_PCI(node, 3), 0x188, ~BIT(4), BIT(4));
		} else {
			/* CstateMsgDis = 0 */
			set_pci_node_reg(NODE_PCI(node, 4), 0x128, ~BIT(31), 0);
		}

		/* MTC1eEn = 1 */
		set_pci_node_reg(NODE_PCI(node, 3), 0xd4, ~BIT(13), BIT(13));
	}
}

struct ht_link_state {
	u8 link_real;
	u8 ganged;
	u8 iolink;
	u8 probe_filter_enabled;
	/* Link Base Channel Buffer Count */
	u8 isoc_rsp_data;
	u8 isoc_np_req_data;
	u8 isoc_rsp_cmd;
	u8 isoc_preq;
	u8 isoc_np_req_cmd;
	u8 free_data;
	u8 free_cmd;
	u8 rsp_data;
	u8 np_req_data;
	u8 probe_cmd;
	u8 rsp_cmd;
	u8 preq;
	u8 np_req_cmd;
	/* Link to XCS Token Counts */
	u8 isoc_rsp_tok_1;
	u8 isoc_preq_tok_1;
	u8 isoc_req_tok_1;
	u8 probe_tok_1;
	u8 rsp_tok_1;
	u8 preq_tok_1;
	u8 req_tok_1;
	u8 isoc_rsp_tok_0;
	u8 isoc_preq_tok_0;
	u8 isoc_req_tok_0;
	u8 free_tokens;
	u8 probe_tok_0;
	u8 rsp_tok_0;
	u8 preq_tok_0;
	u8 req_tok_0;
};

static void set_l3_free_list_mbc(u8 node)
{
	u8 cu_enabled;
	u8 compute_unit_buffer_count;

	/* Determine the number of active compute units on this node */
	cu_enabled = pci_read_config32(NODE_PCI(node, 5), 0x80) & 0xf;

	switch (cu_enabled) {
	case 0x1:
		compute_unit_buffer_count = 0x1c;
		break;
	case 0x3:
		compute_unit_buffer_count = 0x18;
		break;
	case 0x7:
		compute_unit_buffer_count = 0x14;
		break;
	default:
		compute_unit_buffer_count = 0x10;
		break;
	}

	/* L3FreeListCBC = compute_unit_buffer_count */
	set_pci_node_reg(NODE_PCI(node, 3), 0x1a0, ~(0x1f << 4),
				(compute_unit_buffer_count << 4));
}

static u8 is_link_ganged(u8 node, u8 link_real)
{
	return !!(pci_read_config32(NODE_PCI(node, 0), (link_real << 2) + 0x170) & 0x1);
}


static u8 is_iolink(u8 node, u8 offset)
{
	return !!(AMD_checkLinkType(node, offset) & HTPHY_LINKTYPE_NONCOHERENT);
}

static void write_ht_link_buf_counts(u8 node, struct ht_link_state *link_state)
{
	u32 dword;

	dword = pci_read_config32(NODE_PCI(node, 0), (link_state->link_real * 0x20) + 0x94);
	dword &= ~(0x3 << 27);			/* IsocRspData = isoc_rsp_data */
	dword |= ((link_state->isoc_rsp_data & 0x3) << 27);
	dword &= ~(0x3 << 25);			/* IsocNpReqData = isoc_np_req_data */
	dword |= ((link_state->isoc_np_req_data & 0x3) << 25);
	dword &= ~(0x7 << 22);			/* IsocRspCmd = isoc_rsp_cmd */
	dword |= ((link_state->isoc_rsp_cmd & 0x7) << 22);
	dword &= ~(0x7 << 19);			/* IsocPReq = isoc_preq */
	dword |= ((link_state->isoc_preq & 0x7) << 19);
	dword &= ~(0x7 << 16);			/* IsocNpReqCmd = isoc_np_req_cmd */
	dword |= ((link_state->isoc_np_req_cmd & 0x7) << 16);
	pci_write_config32(NODE_PCI(node, 0), (link_state->link_real * 0x20) + 0x94, dword);

	dword = pci_read_config32(NODE_PCI(node, 0), (link_state->link_real * 0x20) + 0x90);
	dword &= ~(0x1 << 31);			/* LockBc = 0x1 */
	dword |= (0x1 << 31);
	dword &= ~(0x7 << 25);			/* FreeData = free_data */
	dword |= ((link_state->free_data & 0x7) << 25);
	dword &= ~(0x1f << 20);			/* FreeCmd = free_cmd */
	dword |= ((link_state->free_cmd & 0x1f) << 20);
	dword &= ~(0x3 << 18);			/* RspData = rsp_data */
	dword |= ((link_state->rsp_data & 0x3) << 18);
	dword &= ~(0x3 << 16);			/* NpReqData = np_req_data */
	dword |= ((link_state->np_req_data & 0x3) << 16);
	dword &= ~(0xf << 12);			/* ProbeCmd = probe_cmd */
	dword |= ((link_state->probe_cmd & 0xf) << 12);
	dword &= ~(0xf << 8);			/* RspCmd = rsp_cmd */
	dword |= ((link_state->rsp_cmd & 0xf) << 8);
	dword &= ~(0x7 << 5);			/* PReq = preq */
	dword |= ((link_state->preq & 0x7) << 5);
	dword &= ~(0x1f << 0);			/* NpReqCmd = np_req_cmd */
	dword |= ((link_state->np_req_cmd & 0x1f) << 0);
	pci_write_config32(NODE_PCI(node, 0), (link_state->link_real * 0x20) + 0x90, dword);
}

static void set_ht_link_buf_counts(u8 node, u8 link, struct ht_link_state *link_state)
{
	u8 offset;

	if (!amd_cpu_find_capability(node, link, &offset))
		return;

	link_state->link_real = (offset - 0x80) / 0x20;
	link_state->iolink = is_iolink(node, offset);
	link_state->ganged = is_link_ganged(node, link_state->link_real);
	link_state->probe_filter_enabled = is_dual_node(node);
	/* Common settings for all links and system configurations */
	link_state->free_cmd = 8;
	link_state->isoc_rsp_data = 0;
	link_state->isoc_np_req_data = 0;
	link_state->isoc_rsp_cmd = 0;
	link_state->isoc_preq = 0;
	link_state->isoc_np_req_cmd = 1;

	if (!link_state->iolink) {
		link_state->free_data = 0;
		link_state->rsp_data = 3;
		link_state->np_req_data = 3;
		link_state->rsp_cmd = 9;
		link_state->preq = 2;
		if (link_state->probe_filter_enabled) {
			link_state->probe_cmd = 4;
			link_state->np_req_cmd = 8;
		} else {
			link_state->probe_cmd = 8;
			link_state->np_req_cmd = 4;
		}
	} else { /* is IO link */
		link_state->rsp_data = 1;
		link_state->probe_cmd = 0;
		link_state->rsp_cmd = 2;
		if (link_state->ganged) {
			link_state->free_data = 0;
			link_state->np_req_data = 0;
			link_state->preq = 7;
			link_state->np_req_cmd = 14;
		} else { /* iolink && !ganged */
			/* FIXME
			 * This is an educated guess as the BKDG does not specify
			 * the appropriate buffer counts for this case!
			 */
			link_state->free_data = 1;
			link_state->np_req_data = 1;
			link_state->preq = 4;
			link_state->np_req_cmd = 12;
		}
	}

	write_ht_link_buf_counts(node, link_state);
}

static void write_ht_link_to_xcs_token_counts(u8 node, struct ht_link_state *link_state)
{
	u32 dword;

	dword = pci_read_config32(NODE_PCI(node, 3), (link_state->link_real << 2) + 0x148);
	dword &= ~(0x3 << 30);			/* FreeTok[3:2] = free_tokens[3:2] */
	dword |= (((link_state->free_tokens >> 2) & 0x3) << 30);
	dword &= ~(0x1 << 28);			/* IsocRspTok1 = isoc_rsp_tok_1 */
	dword |= (((link_state->isoc_rsp_tok_1) & 0x1) << 28);
	dword &= ~(0x1 << 26);			/* IsocPreqTok1 = isoc_preq_tok_1 */
	dword |= (((link_state->isoc_preq_tok_1) & 0x1) << 26);
	dword &= ~(0x1 << 24);			/* IsocReqTok1 = isoc_req_tok_1 */
	dword |= (((link_state->isoc_req_tok_1) & 0x1) << 24);
	dword &= ~(0x3 << 22);			/* ProbeTok1 = probe_tok_1 */
	dword |= (((link_state->probe_tok_1) & 0x3) << 22);
	dword &= ~(0x3 << 20);			/* RspTok1 = rsp_tok_1 */
	dword |= (((link_state->rsp_tok_1) & 0x3) << 20);
	dword &= ~(0x3 << 18);			/* PReqTok1 = preq_tok_1 */
	dword |= (((link_state->preq_tok_1) & 0x3) << 18);
	dword &= ~(0x3 << 16);			/* ReqTok1 = req_tok_1 */
	dword |= (((link_state->req_tok_1) & 0x3) << 16);
	dword &= ~(0x3 << 14);			/* FreeTok[1:0] = free_tokens[1:0] */
	dword |= (((link_state->free_tokens) & 0x3) << 14);
	dword &= ~(0x3 << 12);			/* IsocRspTok0 = isoc_rsp_tok_0 */
	dword |= (((link_state->isoc_rsp_tok_0) & 0x3) << 12);
	dword &= ~(0x3 << 10);			/* IsocPreqTok0 = isoc_preq_tok_0 */
	dword |= (((link_state->isoc_preq_tok_0) & 0x3) << 10);
	dword &= ~(0x3 << 8);			/* IsocReqTok0 = isoc_req_tok_0 */
	dword |= (((link_state->isoc_req_tok_0) & 0x3) << 8);
	dword &= ~(0x3 << 6);			/* ProbeTok0 = probe_tok_0 */
	dword |= (((link_state->probe_tok_0) & 0x3) << 6);
	dword &= ~(0x3 << 4);			/* RspTok0 = rsp_tok_0 */
	dword |= (((link_state->rsp_tok_0) & 0x3) << 4);
	dword &= ~(0x3 << 2);			/* PReqTok0 = preq_tok_0 */
	dword |= (((link_state->preq_tok_0) & 0x3) << 2);
	dword &= ~(0x3 << 0);			/* ReqTok0 = req_tok_0 */
	dword |= (((link_state->req_tok_0) & 0x3) << 0);
	pci_write_config32(NODE_PCI(node, 3), (link_state->link_real << 2) + 0x148, dword);
}

static void set_ht_link_to_xcs_token_counts(u8 node, u8 link, struct ht_link_state *link_state)
{
	u8 offset;
	/* FIXME
	 * This should be configurable
	 */
	u8 sockets = 2;
	u8 sockets_populated = 2;

	if (!amd_cpu_find_capability(node, link, &offset))
		return;

	/* Set defaults */
	link_state->isoc_rsp_tok_1 = 0;
	link_state->isoc_preq_tok_1 = 0;
	link_state->isoc_rsp_tok_0 = 0;
	link_state->isoc_preq_tok_0 = 0;
	link_state->isoc_req_tok_0 = 1;
	link_state->isoc_req_tok_1 = 0;
	link_state->probe_tok_1 = !link_state->ganged;
	link_state->rsp_tok_1 = !link_state->ganged;
	link_state->preq_tok_1 = !link_state->ganged;
	link_state->req_tok_1 = !link_state->ganged;
	link_state->probe_tok_0 = ((link_state->ganged) ? 2 : 1);
	link_state->rsp_tok_0 = ((link_state->ganged) ? 2 : 1);
	link_state->preq_tok_0 = ((link_state->ganged) ? 2 : 1);
	link_state->req_tok_0 = ((link_state->ganged) ? 2 : 1);
	link_state->free_tokens = 0;

	if (link_state->iolink && link_state->ganged
				&& sockets == 4 && sockets_populated == 4) {
		link_state->isoc_req_tok_0 = 2;
	}

	if (!link_state->iolink && !link_state->ganged
				&& sockets >= 2 && sockets_populated >= 2) {
		link_state->isoc_req_tok_1 = 1;
	}

	if (link_state->iolink && link_state->ganged) {
		if (!is_dual_node(node)) {
			link_state->probe_tok_0 = 0;
		} else if ((sockets == 1) || (sockets == 2)
				|| ((sockets == 4) && (sockets_populated == 2))) {
			link_state->probe_tok_0 = 0;
		}
	}

	if (!link_state->iolink && link_state->ganged
				&& ((sockets_populated == 2) && (sockets >= 2))) {
		link_state->probe_tok_0 = 1;
	}

	if (!link_state->iolink && link_state->ganged && is_dual_node(node)
			&& (sockets == 4) && (sockets_populated == 4)) {
		link_state->rsp_tok_0 = 1;
		link_state->preq_tok_0 = 1;
	}

	if (!link_state->iolink && link_state->ganged && link_state->probe_filter_enabled) {
		link_state->rsp_tok_0 = 2;
	}

	if (link_state->ganged && !is_dual_node(node)) {
		link_state->free_tokens = 3;
	}

	if (!link_state->iolink && !link_state->ganged && (sockets_populated == 2)) {
		link_state->free_tokens = sockets;
	}

	write_ht_link_to_xcs_token_counts(node, link_state);
}

static void set_sri_to_xcs_token_counts(u8 node)
{
	/* Set up the SRI to XCS Token Count */
	u8 free_tok;
	u8 up_rsp_tok;
	u8 probe_filter_enabled = is_dual_node(node);
	u32 dword;
	/* FIXME
	 * This should be configurable
	 */
	u8 sockets = 2;
	u8 sockets_populated = 2;

	/* Set defaults */
	free_tok = 0xa;
	up_rsp_tok = 0x3;

	if (!is_dual_node(node)) {
		free_tok = 0xa;
		up_rsp_tok = 0x3;
	} else {
		if ((sockets == 1) || ((sockets == 2) && (sockets_populated == 1))) {
			if (probe_filter_enabled)
				free_tok = 0x9;
			else
				free_tok = 0xa;
			up_rsp_tok = 0x3;
		} else if ((sockets == 2) && (sockets_populated == 2)) {
			free_tok = 0xb;
			up_rsp_tok = 0x1;
		} else if ((sockets == 4) && (sockets_populated == 2)) {
			free_tok = 0xa;
			up_rsp_tok = 0x3;
		} else if ((sockets == 4) && (sockets_populated == 4)) {
			free_tok = 0x9;
			up_rsp_tok = 0x1;
		}
	}

	dword = pci_read_config32(NODE_PCI(node, 3), 0x140);
	dword &= ~(0xf << 20);			/* FreeTok = free_tok */
	dword |= ((free_tok & 0xf) << 20);
	dword &= ~(0x3 << 8);			/* UpRspTok = up_rsp_tok */
	dword |= ((up_rsp_tok & 0x3) << 8);
	pci_write_config32(NODE_PCI(node, 3), 0x140, dword);
}

static u8 check_if_isochronous_link_present(u8 node)
{
	u8 isochronous;
	u8 isochronous_link_present;
	u8 offset;
	u8 link;

	isochronous_link_present = 0;

	for (link = 0; link < 4; link++) {
		if (amd_cpu_find_capability(node, link, &offset)) {
			isochronous = pci_read_config32(NODE_PCI(node, 0), offset + 4);
			isochronous = (isochronous >> 12) & 0x1;

			if (isochronous)
				isochronous_link_present = 1;
		}
	}

	return isochronous_link_present;
}

static void setup_isochronous_ht_link(u8 node)
{
	u8 free_tok;
	u8 up_rsp_cbc;
	u8 isoc_preq_cbc;
	u8 isoc_preq_tok;
	u8 xbar_to_sri_free_list_cbc;
	u32 dword;

	/* Adjust buffer counts */
	dword = pci_read_config32(NODE_PCI(node, 3), 0x70);
	isoc_preq_cbc = (dword >> 24) & 0x7;
	up_rsp_cbc = (dword >> 16) & 0x7;
	up_rsp_cbc--;
	isoc_preq_cbc++;
	dword &= ~(0x7 << 24);		/* IsocPreqCBC = isoc_preq_cbc */
	dword |= ((isoc_preq_cbc & 0x7) << 24);
	dword &= ~(0x7 << 16);		/* UpRspCBC = up_rsp_cbc */
	dword |= ((up_rsp_cbc & 0x7) << 16);
	pci_write_config32(NODE_PCI(node, 3), 0x70, dword);

	dword = pci_read_config32(NODE_PCI(node, 3), 0x74);
	isoc_preq_cbc = (dword >> 24) & 0x7;
	isoc_preq_cbc++;
	dword &= ~(0x7 << 24);		/* IsocPreqCBC = isoc_preq_cbc */
	dword |= (isoc_preq_cbc & 0x7) << 24;
	pci_write_config32(NODE_PCI(node, 3), 0x74, dword);

	dword = pci_read_config32(NODE_PCI(node, 3), 0x7c);
	xbar_to_sri_free_list_cbc = dword & 0x1f;
	xbar_to_sri_free_list_cbc--;
	dword &= ~0x1f;			/* Xbar2SriFreeListCBC = xbar_to_sri_free_list_cbc */
	dword |= xbar_to_sri_free_list_cbc & 0x1f;
	pci_write_config32(NODE_PCI(node, 3), 0x7c, dword);

	dword = pci_read_config32(NODE_PCI(node, 3), 0x140);
	free_tok = (dword >> 20) & 0xf;
	isoc_preq_tok = (dword >> 14) & 0x3;
	free_tok--;
	isoc_preq_tok++;
	dword &= ~(0xf << 20);		/* FreeTok = free_tok */
	dword |= ((free_tok & 0xf) << 20);
	dword &= ~(0x3 << 14);		/* IsocPreqTok = isoc_preq_tok */
	dword |= ((isoc_preq_tok & 0x3) << 14);
	pci_write_config32(NODE_PCI(node, 3), 0x140, dword);
}

static void setup_ht_buffer_allocation(u8 node)
{
	struct ht_link_state link_state[4];
	u8 link;

	set_l3_free_list_mbc(node);

	for (link = 0; link < 4; link++) {
		set_ht_link_buf_counts(node, link, &link_state[link]);
	}

	for (link = 0; link < 4; link++) {
		set_ht_link_to_xcs_token_counts(node, link, &link_state[link]);
	}

	set_sri_to_xcs_token_counts(node);

	if (check_if_isochronous_link_present(node))
		setup_isochronous_ht_link(node);

}

static void amd_setup_psivid_d(u32 platform_type, u8 node)
{
	u32 dword;
	int i;
	msr_t msr;

	if (platform_type & (AMD_PTYPE_MOB | AMD_PTYPE_DSK)) {

		/* The following code sets the PSIVID to the lowest support P state
		 * assuming that the VID for the lowest power state is below
		 * the VDD voltage regulator threshold. (This also assumes that there
		 * is a Pstate lower than P0)
		 */

		for (i = 4; i >= 0; i--) {
			msr = rdmsr(PSTATE_0_MSR + i);
			/* Pstate valid? */
			if (msr.hi & BIT(31)) {
				dword = pci_read_config32(NODE_PCI(i, 3), 0xA0);
				dword &= ~0x7F;
				dword |= (msr.lo >> 9) & 0x7F;
				pci_write_config32(NODE_PCI(i, 3), 0xA0, dword);
				break;
			}
		}
	}
}

void cpuSetAMDPCI(u8 node)
{
	/* This routine loads the CPU with default settings in fam10_pci_default
	 * table . It must be run after Cache-As-RAM has been enabled, and
	 * Hypertransport initialization has taken place. Also note
	 * that it is run for the first core on each node
	 */
	u32 platform;
	u64 revision;

	printk(BIOS_DEBUG, "cpuSetAMDPCI %02d", node);

	revision = get_logical_CPUID(node);
	platform = get_platform_type();

	/* Set PSIVID offset which is not table driven */
	amd_setup_psivid_d(platform, node);
	configure_pci_defaults(node);
	configure_ht_stop_tristate(node);
	set_ht_phy_defaults(node);

	/* FIXME: add UMA support and programXbarToSriReg(); */

	AMD_Errata281(node, revision, platform);

	/* FIXME: if the dct phy doesn't init correct it needs to reset.
	 * if (revision & (AMD_DR_B2 | AMD_DR_B3))
	 * dctPhyDiag();
	 */

	configure_message_triggered_c1e(node);

	if (revision & AMD_FAM15_ALL)
		setup_ht_buffer_allocation(node);

	printk(BIOS_DEBUG, " done\n");
}

static void enable_memory_speed_boost(u8 node_id)
{
	msr_t msr;
	u32 f3x1fc = pci_read_config32(NODE_PCI(node_id, 3), 0x1fc);
	msr = rdmsr(FP_CFG_MSR);
	msr.hi &= ~(0x7 << (42-32));			/* DiDtCfg4 */
	msr.hi |= (((f3x1fc >> 17) & 0x7) << (42-32));
	msr.hi &= ~(0x1 << (41-32));			/* DiDtCfg5 */
	msr.hi |= (((f3x1fc >> 22) & 0x1) << (41-32));
	msr.hi &= ~(0x1 << (40-32));			/* DiDtCfg3 */
	msr.hi |= (((f3x1fc >> 16) & 0x1) << (40-32));
	msr.hi &= ~(0x7 << (32-32));			/* DiDtCfg1 (1) */
	msr.hi |= (((f3x1fc >> 11) & 0x7) << (32-32));
	msr.lo &= ~(0x1f << 27);			/* DiDtCfg1 (2) */
	msr.lo |= (((f3x1fc >> 6) & 0x1f) << 27);
	msr.lo &= ~(0x3 << 25);				/* DiDtCfg2 */
	msr.lo |= (((f3x1fc >> 14) & 0x3) << 25);
	msr.lo &= ~(0x1f << 18);			/* DiDtCfg0 */
	msr.lo |= (((f3x1fc >> 1) & 0x1f) << 18);
	msr.lo &= ~(0x1 << 16);				/* DiDtMode */
	msr.lo |= ((f3x1fc & 0x1) << 16);
	wrmsr(FP_CFG_MSR, msr);

	if (get_uint_option("experimental_memory_speed_boost", 0)) {
		msr = rdmsr(BU_CFG3_MSR);
		msr.lo |= (0x3 << 20);			/* PfcStrideMul = 0x3 */
		wrmsr(BU_CFG3_MSR, msr);
	}
}

static void enable_message_triggered_c1e(u64 revision)
{
	msr_t msr;
	/* Set up message triggered C1E */
	msr = rdmsr(MSR_INTPEND);
	msr.lo &= ~0xffff;		/* IOMsgAddr = ACPI_PM_EVT_BLK */
	msr.lo |= ACPI_PM_EVT_BLK & 0xffff;
	msr.lo |= (0x1 << 29);		/* BmStsClrOnHltEn = 1 */
	if (revision & AMD_DR_GT_D0) {
		msr.lo &= ~(0x1 << 28);	/* C1eOnCmpHalt = 0 */
		msr.lo &= ~(0x1 << 27);	/* SmiOnCmpHalt = 0 */
	}
	wrmsr(MSR_INTPEND, msr);

	msr = rdmsr(HWCR_MSR);
	msr.lo |= (0x1 << 12);		/* HltXSpCycEn = 1 */
	wrmsr(HWCR_MSR, msr);
}

static void enable_cpu_c_states(void)
{
	if (!CONFIG(HAVE_ACPI_TABLES))
		return;

	if (get_uint_option("cpu_c_states", 0)) {
		/* Set up the C-state base address */
		msr_t c_state_addr_msr;
		c_state_addr_msr = rdmsr(MSR_CSTATE_ADDRESS);
		c_state_addr_msr.lo = ACPI_CPU_P_LVL2;
		wrmsr(MSR_CSTATE_ADDRESS, c_state_addr_msr);
	}
}

static void cpb_enable_disable(void)
{
	if (!get_uint_option("cpu_core_boost", 1)) {
		msr_t msr;
		/* Disable Core Performance Boost */
		msr = rdmsr(HWCR_MSR);
		msr.lo |= (0x1 << 25);		/* CpbDis = 1 */
		wrmsr(HWCR_MSR, msr);
	}
}

static void AMD_Errata298(void)
{
	/* Workaround for L2 Eviction May Occur during operation to
	 * set Accessed or dirty bit.
	 */

	msr_t msr;
	u8 i;
	u8 affectedRev = 0;
	u8 nodes = get_nodes();

	/* For each core we need to check for a "broken" node */
	for (i = 0; i < nodes; i++) {
		if (get_logical_CPUID(i) & (AMD_DR_B0 | AMD_DR_B1 | AMD_DR_B2)) {
			affectedRev = 1;
			break;
		}
	}

	if (affectedRev) {
		msr = rdmsr(HWCR_MSR);
		msr.lo |= 0x08;	/* Set TlbCacheDis bit[3] */
		wrmsr(HWCR_MSR, msr);

		msr = rdmsr(BU_CFG_MSR);
		msr.lo |= 0x02;	/* Set TlbForceMemTypeUc bit[1] */
		wrmsr(BU_CFG_MSR, msr);

		msr = rdmsr(OSVW_ID_Length);
		msr.lo |= 0x01;	/* OS Visible Workaround - MSR */
		wrmsr(OSVW_ID_Length, msr);

		msr = rdmsr(OSVW_Status);
		msr.lo |= 0x01;	/* OS Visible Workaround - MSR */
		wrmsr(OSVW_Status, msr);
	}

	if (!affectedRev && (get_logical_CPUID(0xFF) & AMD_DR_B3)) {
		msr = rdmsr(OSVW_ID_Length);
		msr.lo |= 0x01;	/* OS Visible Workaround - MSR */
		wrmsr(OSVW_ID_Length, msr);

	}
}

void cpuSetAMDMSR(u8 node_id)
{
	/* This routine loads the CPU with default settings in fam10_msr_default
	 * table . It must be run after Cache-As-RAM has been enabled, and
	 * Hypertransport initialization has taken place. Also note
	 * that it is run on the current processor only, and only for the current
	 * processor core.
	 */
	msr_t msr;
	u8 i;
	u32 platform;
	u64 revision;

	printk(BIOS_DEBUG, "cpuSetAMDMSR ");

	revision = get_logical_CPUID(0xff);
	platform = get_platform_type();

	for (i = 0; i < ARRAY_SIZE(fam10_msr_default); i++) {
		if ((fam10_msr_default[i].revision & revision)
				&& (fam10_msr_default[i].platform & platform)) {
			msr = rdmsr(fam10_msr_default[i].msr);
			msr.hi &= ~fam10_msr_default[i].mask_hi;
			msr.hi |= fam10_msr_default[i].data_hi;
			msr.lo &= ~fam10_msr_default[i].mask_lo;
			msr.lo |= fam10_msr_default[i].data_lo;
			wrmsr(fam10_msr_default[i].msr, msr);
		}
	}
	AMD_Errata298();

	/* Revision C0 and above */
	if (revision & AMD_OR_C0)
		enable_memory_speed_boost(node_id);

	/* SB700 or SB800 */
	if (CONFIG(SOUTHBRIDGE_AMD_SB700)) {
		if (revision & (AMD_DR_GT_D0 | AMD_FAM15_ALL))
			enable_message_triggered_c1e(revision);

		if (revision & (AMD_DR_Ex | AMD_FAM15_ALL))
			enable_cpu_c_states();
	}

	if (revision & AMD_FAM15_ALL)
		cpb_enable_disable();

	printk(BIOS_DEBUG, " done\n");
}
