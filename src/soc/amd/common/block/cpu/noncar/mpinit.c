/* SPDX-License-Identifier: GPL-2.0-only */

#include <acpi/acpi.h>
#include <amdblocks/psp.h>
#include <amdblocks/iomap.h>
#include <amdblocks/lpc.h>
#include <console/console.h>
#include <cpu/x86/mp.h>
#include <cpu/x86/mtrr.h>
#include <cpu/x86/smm.h>
#include <dasharo/options.h>
#include <device/device.h>
#include <drivers/efi/capsules.h>
#include <types.h>

void mp_init_cpus(struct bus *cpu_bus)
{
	uintptr_t capsule_base = 0;
	size_t capsule_size = 0;
	bool smm_bwp_enabled = is_smm_bwp_permitted();

	extern const struct mp_ops amd_mp_ops_with_smm;
	if (mp_init_with_smm(cpu_bus, &amd_mp_ops_with_smm) != CB_SUCCESS)
		die_with_post_code(POSTCODE_HW_INIT_FAILURE,
				"mp_init_with_smm failed. Halting.\n");

	/* pre_mp_init made the flash not cacheable. Reset to WP for performance. */
	mtrr_use_temp_range(FLASH_BELOW_4GB_MAPPING_REGION_BASE,
			    FLASH_BELOW_4GB_MAPPING_REGION_SIZE, MTRR_TYPE_WRPROT);

	if (CONFIG(SOC_AMD_COMMON_BLOCK_SPI_MMAP_USE_ROM3)) {
		size_t rom3_size = 0;
		uint64_t rom3_start = lpc_get_rom3_region(&rom3_size);

		if (rom3_start && rom3_size) {
			/* The flash is now no longer cacheable. Reset to WP for performance. */
			rom3_size = 1 << log2_ceil(rom3_size);
			mtrr_use_temp_range(rom3_start, rom3_size, MTRR_TYPE_WRPROT);
		}
	}

	if (smm_bwp_enabled && CONFIG(SOC_AMD_COMMON_BLOCK_PSP_ROM_ARMOR3)) {
		/* Parse EFI capsules */
		efi_parse_capsules(&capsule_base, &capsule_size);
		psp_rom_armor_init(capsule_base && capsule_size);
	}

	/* SMMINFO only needs to be set up when booting from S5 */
	if (!acpi_is_wakeup_s3())
		apm_control(APM_CNT_SMMINFO);

	/* For ROM Armor 1, enabling SMM-only mode must happen after SMM_INFO */
	if (smm_bwp_enabled && CONFIG(SOC_AMD_COMMON_BLOCK_PSP_ROM_ARMOR1)) {
		/* Parse EFI capsules */
		efi_parse_capsules(&capsule_base, &capsule_size);
		psp_rom_armor_init(capsule_base && capsule_size);
	}
}
