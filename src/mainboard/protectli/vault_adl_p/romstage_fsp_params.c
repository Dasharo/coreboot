/* SPDX-License-Identifier: GPL-2.0-only */

#include <cbfs.h>
#include <cpu/x86/msr.h>
#include <fsp/api.h>
#include <soc/romstage.h>
#include <soc/meminit.h>
#include <soc/gpio.h>
#include <security/intel/cbnt/cbnt.h>
#include <security/intel/txt/txt.h>

#include "gpio.h"

static const struct mb_cfg ddr5_mem_config = {
	.type = MEM_TYPE_DDR5,
	.ect = true, /* Early Command Training */
	.UserBd = BOARD_TYPE_MOBILE,
	.LpDdrDqDqsReTraining = 1,
};

static const struct mem_spd dimm_module_spd_info = {
	.topo = MEM_TOPO_DIMM_MODULE,
	.smbus = {
		[0] = {
			.addr_dimm[0] = 0x50,
		},
		[1] = {
			.addr_dimm[0] = 0x52,
		},
	},
};

void mainboard_memory_init_params(FSPM_UPD *memupd)
{
	const struct pad_config *pads;
	size_t num;

	memcfg_init(memupd, &ddr5_mem_config, &dimm_module_spd_info, false);

	pads = board_gpio_table(&num);
	gpio_configure_pads(pads, num);

	// Set primary display to internal graphics
	memupd->FspmConfig.PrimaryDisplay = 0;
	memupd->FspmConfig.DmiMaxLinkSpeed = 4;
	memupd->FspmConfig.CpuPcieRpClockReqMsgEnable[0] = 0;
	memupd->FspmConfig.CpuPcieRpClockReqMsgEnable[1] = 0;
	memupd->FspmConfig.CpuPcieRpClockReqMsgEnable[2] = 0;

	/* Limit the max speed for memory compatibility */
	memupd->FspmConfig.DdrFreqLimit = 4200;

/* Use pre-processor because CONFIG_INTEL_TXT_CBFS_BIOS_ACM is not defined otherwise */
#if CONFIG(INTEL_TXT)
	size_t acm_size = 0;
	uintptr_t acm_base;

	if (CONFIG(INTEL_TXT)) {
		intel_txt_log_spad();

		if (CONFIG(INTEL_CBNT_LOGGING))
			intel_cbnt_log_registers();

		if (CONFIG(INTEL_TXT_LOGGING)) {
			intel_txt_log_bios_acm_error();
			txt_dump_chipset_info();
		}
	}

	acm_base = (uintptr_t)cbfs_map(CONFIG_INTEL_TXT_CBFS_BIOS_ACM, &acm_size);

	msr_t msr = rdmsr(IA32_FEATURE_CONTROL);
	printk(BIOS_DEBUG, "IA32_FEATURE_CONTROL: %08x %08x\n", msr.hi, msr.lo);

	memupd->FspmConfig.VmxEnable = 1;
	memupd->FspmConfig.TxtImplemented = 1;
	memupd->FspmConfig.Txt = 1;
	memupd->FspmConfig.SinitMemorySize = CONFIG_INTEL_TXT_SINIT_SIZE;
	memupd->FspmConfig.TxtHeapMemorySize = CONFIG_INTEL_TXT_HEAP_SIZE;
	memupd->FspmConfig.TxtDprMemorySize = CONFIG_INTEL_TXT_DPR_SIZE << 20;
	memupd->FspmConfig.TxtDprMemoryBase = 1; // Set to non-zero, FSP will update it
	memupd->FspmConfig.BiosAcmBase = acm_base;
	memupd->FspmConfig.BiosAcmSize = acm_size;
	memupd->FspmConfig.ApStartupBase = 1;  // Set to non-zero, FSP does NULL check
#endif
}
