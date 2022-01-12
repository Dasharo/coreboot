/* SPDX-License-Identifier: GPL-2.0-only */

#include <console/console.h>
#include <cpu/amd/amddefs.h>
#include <cpu/amd/msr.h>
#include <types.h>

#include "common.h"
#include "nums.h"

u32 read_nb_cfg_54(void)
{
	msr_t msr;
	msr = rdmsr(NB_CFG_MSR);
	return (msr.hi >> (54-32)) & 1;
}

u32 get_initial_apicid(void)
{
	return (cpuid_ebx(1) >> 24) & 0xff;
}

u32 get_core_num_in_bsp(u32 nodeid)
{
	u32 dword;
	if (is_fam15h()) {
		/* Family 15h moved CmpCap to F5x84 [7:0] */
		dword = pci_read_config32(NODE_PCI(nodeid, 5), 0x84);
		dword &= 0xff;
	} else {
		dword = pci_read_config32(NODE_PCI(nodeid, 3), 0xe8);
		dword >>= 12;
		/* Bit 15 is CmpCap[2] since Revision D. */
		if ((cpuid_ecx(0x80000008) & 0xff) > 3)
			dword = ((dword & 8) >> 1) | (dword & 3);
		else
			dword &= 3;
	}
	return dword;
}

u8 set_apicid_cpuid_lo(void)
{
	// set the NB_CFG[54]=1; why the OS will be happy with that ???
	msr_t msr;
	msr = rdmsr(NB_CFG_MSR);
	msr.hi |= (1 << (54 - 32)); // InitApicIdCpuIdLo
	wrmsr(NB_CFG_MSR, msr);

	return 1;
}

void set_EnableCf8ExtCfg(void)
{
	if (CONFIG(PCI_IO_CFG_EXT))
		return;

	// set the NB_CFG_MSR[46]=1;
	msr_t msr;
	msr = rdmsr(NB_CFG_MSR);
	// EnableCf8ExtCfg: We need that to access CONFIG_PCI_IO_CFG_EXT 4K range
	msr.hi |= (1 << (46 - 32));
	wrmsr(NB_CFG_MSR, msr);
}

u64 get_logical_CPUID(u32 node)
{
	/* Converts the CPUID to a logical ID MASK that is used to check
	 CPU version support versions */
	u32 val, valx;
	u32 family, model, stepping;
	u64 ret;

	if (node == 0xFF) { /* current node */
		val = cpuid_eax(0x80000001);
	} else {
		val = pci_read_config32(NODE_PCI(node, 3), 0xfc);
	}

	family = ((val >> 8) & 0x0f) + ((val >> 20) & 0xff);
	model = ((val >> 4) & 0x0f) | ((val >> (16-4)) & 0xf0);
	stepping = val & 0x0f;

	valx = (family << 12) | (model << 4) | (stepping);

	switch (valx) {
	case 0x10000:
		ret = AMD_DR_A0A;
		break;
	case 0x10001:
		ret = AMD_DR_A1B;
		break;
	case 0x10002:
		ret = AMD_DR_A2;
		break;
	case 0x10020:
		ret = AMD_DR_B0;
		break;
	case 0x10021:
		ret = AMD_DR_B1;
		break;
	case 0x10022:
		ret = AMD_DR_B2;
		break;
	case 0x10023:
		ret = AMD_DR_B3;
		break;
	case 0x10042:
		ret = AMD_RB_C2;
		break;
	case 0x10043:
		ret = AMD_RB_C3;
		break;
	case 0x10062:
		ret = AMD_DA_C2;
		break;
	case 0x10063:
		ret = AMD_DA_C3;
		break;
	case 0x10080:
		ret = AMD_HY_D0;
		break;
	case 0x10081:
	case 0x10091:
		ret = AMD_HY_D1;
		break;
	case 0x100a0:
		ret = AMD_PH_E0;
		break;
	case 0x15012:
	case 0x1501f:
		ret = AMD_OR_B2;
		break;
	case 0x15020:
	case 0x15101:
		ret = AMD_OR_C0;
		break;
	default:
		die("FIXME! CPU Version unknown or not supported! %08x\n", valx);
		ret = 0;
	}

	return ret;
}

u8 get_processor_package_type(void)
{
	u32 BrandId = cpuid_ebx(0x80000001);
	return (u8)((BrandId >> 28) & 0x0f);
}

u32 get_platform_type(void)
{
	u32 ret = 0;

	if (CONFIG(SYSTEM_TYPE_SERVER))
		ret |= AMD_PTYPE_SVR;
	if (CONFIG(SYSTEM_TYPE_DESKTOP))
		ret |= AMD_PTYPE_DSK;
	if (CONFIG(SYSTEM_TYPE_MOBILE))
		ret |= AMD_PTYPE_MOB;

	/* FIXME: add UMA support. */

	/* All Fam10 are multi core */
	ret |= AMD_PTYPE_MC;

	return ret;
}
