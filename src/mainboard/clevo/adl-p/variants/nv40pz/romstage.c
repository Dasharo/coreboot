/* SPDX-License-Identifier: GPL-2.0-only */

#include <cbfs.h>
#include <cpu/x86/msr.h>
#include <security/intel/cbnt/cbnt.h>
#include <security/intel/txt/txt.h>
#include <soc/meminit.h>
#include <soc/romstage.h>

void mainboard_memory_init_params(FSPM_UPD *mupd)
{
	const struct mb_cfg board_cfg = {
		.type = MEM_TYPE_DDR4,
	};
	const struct mem_spd spd_info = {
		.topo = MEM_TOPO_DIMM_MODULE,
		.smbus = {
			[0] = { .addr_dimm[0] = 0x50, },
			[1] = { .addr_dimm[0] = 0x52, },
		},
	};
	const bool half_populated = false;

	mupd->FspmConfig.PchHdaAudioLinkHdaEnable = 1;
	mupd->FspmConfig.DmiMaxLinkSpeed = 4;
	mupd->FspmConfig.GpioOverride = 0;

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

	mupd->FspmConfig.VmxEnable = 1;
	mupd->FspmConfig.TxtImplemented = 1;
	mupd->FspmConfig.Txt = 1;
	mupd->FspmConfig.SinitMemorySize = CONFIG_INTEL_TXT_SINIT_SIZE;
	mupd->FspmConfig.TxtHeapMemorySize = CONFIG_INTEL_TXT_HEAP_SIZE;
	mupd->FspmConfig.TxtDprMemorySize = CONFIG_INTEL_TXT_DPR_SIZE << 20;
	mupd->FspmConfig.TxtDprMemoryBase = 1; // Set to non-zero, FSP will update it
	mupd->FspmConfig.BiosAcmBase = acm_base;
	mupd->FspmConfig.BiosAcmSize = acm_size;
	mupd->FspmConfig.ApStartupBase = 1;  // Set to non-zero, FSP does NULL check
#endif

	memcfg_init(mupd, &board_cfg, &spd_info, half_populated);
}
