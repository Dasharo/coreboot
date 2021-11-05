/* SPDX-License-Identifier: GPL-2.0-only */

int init_fidvid_bsp(u32 bsp_apicid, u32 nodes);
void init_fidvid_ap(u32 apicid, u32 nodeid, u32 coreid);
void init_fidvid_stage2(u32 apicid, u32 nodeid);
void prep_fid_change(void);
