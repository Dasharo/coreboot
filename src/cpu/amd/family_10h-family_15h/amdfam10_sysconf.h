/* SPDX-License-Identifier: GPL-2.0-only */

#ifndef AMDFAM10_SYSCONF_H
#define AMDFAM10_SYSCONF_H

#include <cpu/amd/common/nums.h>
#include <cpu/x86/msr.h>

struct p_state_t {
	unsigned int corefreq;
	unsigned int power;
	unsigned int transition_lat;
	unsigned int busmaster_lat;
	unsigned int control;
	unsigned int status;
};

struct amdfam10_sysconf_t {
	//ht
	unsigned int hc_possible_num;
	unsigned int pci1234[HC_POSSIBLE_NUM];
	unsigned int hcdn[HC_POSSIBLE_NUM];
	unsigned int hcid[HC_POSSIBLE_NUM]; //record ht chain type
	unsigned int sbdn;
	unsigned int sblk;

	unsigned int nodes;
	unsigned int ht_c_num; // we only can have 32 ht chain at most
	// 4-->32: 4:segn, 8:bus_max, 8:bus_min, 4:linkn, 6: nodeid, 2: enable
	unsigned int ht_c_conf_bus[HC_NUMS];
	unsigned int segbit;
	unsigned int hcdn_reg[HC_NUMS]; // it will be used by get_pci1234

	// quad cores all cores in one node should be the same, and p0,..p5
	msr_t msr_pstate[NODE_NUMS * 5];
	unsigned int needs_update_pstate_msrs;

	unsigned int bsp_apicid;
	int enabled_apic_ext_id;
	unsigned int lift_bsp_apicid;
	int apicid_offset;

	void *mb; // pointer for mb related struct
};

void get_bus_conf(void);
void get_pci1234(void);
void get_default_pci1234(int mb_hc_possible);


#endif
