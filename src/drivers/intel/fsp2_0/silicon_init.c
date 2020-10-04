/* SPDX-License-Identifier: GPL-2.0-or-later */

#include <cbfs.h>
#include <cbmem.h>
#include <commonlib/fsp.h>
#include <commonlib/stdlib.h>
#include <console/console.h>
#include <fsp/api.h>
#include <fsp/util.h>
#include <program_loading.h>
#include <soc/intel/common/vbt.h>
#include <stage_cache.h>
#include <string.h>
#include <timestamp.h>
#include <types.h>

struct fsp_header fsps_hdr;

struct fsp_multi_phase_get_number_of_phases_params {
	uint32_t number_of_phases;
	uint32_t phases_executed;
};

/* Callbacks for SoC/Mainboard specific overrides */
void __weak platform_fsp_multi_phase_init_cb(uint32_t phase_index)
{
	/* Leave for the SoC/Mainboard to implement if necessary. */
}

int __weak soc_fsp_multi_phase_init_is_enable(void)
{
	return 1;
}

/* FSP Specification < 2.2 has only 1 stage like FspSiliconInit. FSP specification >= 2.2
 * has multiple stages as below.
 */
enum fsp_silicon_init_phases {
	FSP_SILICON_INIT_API,
	FSP_MULTI_PHASE_SI_INIT_GET_NUMBER_OF_PHASES_API,
	FSP_MULTI_PHASE_SI_INIT_EXECUTE_PHASE_API
};

static void fsps_return_value_handler(enum fsp_silicon_init_phases phases, uint32_t status)
{
	uint8_t postcode;

	/* Handle any reset request returned by FSP-S APIs */
	fsp_handle_reset(status);

	if (status == FSP_SUCCESS)
		return;
	/* Handle all other errors returned by FSP-S APIs */
	/* Assume video failure if attempted to initialize graphics */
	if (CONFIG(RUN_FSP_GOP) && vbt_get())
		postcode = POST_VIDEO_FAILURE;
	else
		postcode = POST_HW_INIT_FAILURE; /* else generic */

	switch (phases) {
	case FSP_SILICON_INIT_API:
		die_with_post_code(postcode, "FspSiliconInit returned with error 0x%08x\n",
				status);
		break;
	case FSP_MULTI_PHASE_SI_INIT_GET_NUMBER_OF_PHASES_API:
		printk(BIOS_SPEW, "FspMultiPhaseSiInit NumberOfPhases returned 0x%08x\n",
				status);
		break;
	case FSP_MULTI_PHASE_SI_INIT_EXECUTE_PHASE_API:
		printk(BIOS_SPEW, "FspMultiPhaseSiInit ExecutePhase returned 0x%08x\n",
				status);
		break;
	default:
		break;
	}
}

static void do_silicon_init(struct fsp_header *hdr)
{
	FSPS_UPD *upd, *supd;
	fsp_silicon_init_fn silicon_init;
	uint32_t status;
	const struct cbmem_entry *logo_entry = NULL;
	fsp_multi_phase_si_init_fn multi_phase_si_init;
	struct fsp_multi_phase_params multi_phase_params;
	struct fsp_multi_phase_get_number_of_phases_params multi_phase_get_number;

	supd = (FSPS_UPD *) (hdr->cfg_region_offset + hdr->image_base);

	if (supd->FspUpdHeader.Signature != FSPS_UPD_SIGNATURE)
		die_with_post_code(POST_INVALID_VENDOR_BINARY,
			"Invalid FSPS signature\n");

	/* Disallow invalid config regions.  Default settings are likely bad
	 * choices for coreboot, and different sized UPD from what the region
	 * allows is potentially a build problem.
	 */
	if (!hdr->cfg_region_size || hdr->cfg_region_size != sizeof(FSPS_UPD))
		die_with_post_code(POST_INVALID_VENDOR_BINARY,
			"Invalid FSPS UPD region\n");

	upd = xmalloc(hdr->cfg_region_size);

	memcpy(upd, supd, hdr->cfg_region_size);

	/* Give SoC/mainboard a chance to populate entries */
	platform_fsp_silicon_init_params_cb(upd);

	/* Populate logo related entries */
	if (CONFIG(FSP2_0_DISPLAY_LOGO))
		logo_entry = soc_load_logo(upd);

	/* Call SiliconInit */
	silicon_init = (void *) (hdr->image_base +
				 hdr->silicon_init_entry_offset);
	fsp_debug_before_silicon_init(silicon_init, supd, upd);

	timestamp_add_now(TS_FSP_SILICON_INIT_START);
	post_code(POST_FSP_SILICON_INIT);
	status = silicon_init(upd);
	timestamp_add_now(TS_FSP_SILICON_INIT_END);
	post_code(POST_FSP_SILICON_EXIT);

	if (logo_entry)
		cbmem_entry_remove(logo_entry);

	fsp_debug_after_silicon_init(status);
	fsps_return_value_handler(FSP_SILICON_INIT_API, status);

	/* Reinitialize CPUs if FSP-S has done MP Init */
	if (CONFIG(USE_INTEL_FSP_MP_INIT))
		do_mpinit_after_fsp();

	if (!CONFIG(PLATFORM_USES_FSP2_2))
		return;

	/* Check if SoC user would like to call Multi Phase Init */
	if (!soc_fsp_multi_phase_init_is_enable())
		return;

	/* Call MultiPhaseSiInit */
	multi_phase_si_init = (void *) (hdr->image_base +
			 hdr->multi_phase_si_init_entry_offset);

	/* Implementing multi_phase_si_init() is optional as per FSP 2.2 spec */
	if (multi_phase_si_init == NULL)
		return;

	post_code(POST_FSP_MULTI_PHASE_SI_INIT_ENTRY);
	timestamp_add_now(TS_FSP_MULTI_PHASE_SI_INIT_START);
	/* Get NumberOfPhases Value */
	multi_phase_params.multi_phase_action = GET_NUMBER_OF_PHASES;
	multi_phase_params.phase_index = 0;
	multi_phase_params.multi_phase_param_ptr = &multi_phase_get_number;
	status = multi_phase_si_init(&multi_phase_params);
	fsps_return_value_handler(FSP_MULTI_PHASE_SI_INIT_GET_NUMBER_OF_PHASES_API, status);

	/* Execute Multi Phase Execution */
	for (int i = 1; i <= multi_phase_get_number.number_of_phases; i++) {
		printk(BIOS_SPEW, "Executing Phase %d of FspMultiPhaseSiInit\n", i);
		/*
		 * Give SoC/mainboard a chance to perform any operation before
		 * Multi Phase Execution
		 */
		platform_fsp_multi_phase_init_cb(i);

		multi_phase_params.multi_phase_action = EXECUTE_PHASE;
		multi_phase_params.phase_index = i;
		multi_phase_params.multi_phase_param_ptr = NULL;
		status = multi_phase_si_init(&multi_phase_params);
		fsps_return_value_handler(FSP_MULTI_PHASE_SI_INIT_EXECUTE_PHASE_API, status);
	}
	timestamp_add_now(TS_FSP_MULTI_PHASE_SI_INIT_END);
	post_code(POST_FSP_MULTI_PHASE_SI_INIT_EXIT);
}

static int fsps_get_dest(const struct fsp_load_descriptor *fspld, void **dest,
				size_t size, const struct region_device *source)
{
	*dest = cbmem_add(CBMEM_ID_REFCODE, size);

	if (*dest == NULL)
		return -1;

	return 0;
}

void fsps_load(bool s3wake)
{
	struct fsp_load_descriptor fspld = {
		.fsp_prog = PROG_INIT(PROG_REFCODE, CONFIG_FSP_S_CBFS),
		.get_destination = fsps_get_dest,
	};
	struct prog *fsps = &fspld.fsp_prog;
	static int load_done;

	if (load_done)
		return;

	if (s3wake && !CONFIG(NO_STAGE_CACHE)) {
		printk(BIOS_DEBUG, "Loading FSPS from stage_cache\n");
		stage_cache_load_stage(STAGE_REFCODE, fsps);
		if (fsp_validate_component(&fsps_hdr, prog_rdev(fsps)) != CB_SUCCESS)
			die("On resume fsps header is invalid\n");
		load_done = 1;
		return;
	}

	if (fsp_load_component(&fspld, &fsps_hdr) != CB_SUCCESS)
		die("FSP-S failed to load\n");

	stage_cache_add(STAGE_REFCODE, fsps);

	load_done = 1;
}

void fsp_silicon_init(bool s3wake)
{
	fsps_load(s3wake);
	do_silicon_init(&fsps_hdr);
}

/* Load bmp and set FSP parameters, fsp_load_logo can be used */
__weak const struct cbmem_entry *soc_load_logo(FSPS_UPD *supd)
{
	return NULL;
}
