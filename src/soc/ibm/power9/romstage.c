/* SPDX-License-Identifier: GPL-2.0-only */

#include <console/console.h>
#include <cpu/power/mvpd.h>
#include <cpu/power/vpd.h>
#include <cpu/power/istep_8.h>
#include <cpu/power/istep_9.h>
#include <cpu/power/istep_10.h>
#include <cpu/power/istep_13.h>
#include <cpu/power/istep_14.h>
#include <cpu/power/proc.h>
#include <drivers/ipmi/ipmi_if.h>
#include <drivers/ipmi/ipmi_ops.h>
#include <program_loading.h>
#include <spd_bin.h>
#include <endian.h>
#include <cbmem.h>
#include <commonlib/bsd/cbmem_id.h>
#include <timestamp.h>

#include "fsi.h"
#include "pci.h"

mcbist_data_t mem_data[MAX_CHIPS];

static void dump_mca_data(mca_data_t *mca)
{
	printk(BIOS_SPEW, "\tCL =      %d\n", mca->cl);
	printk(BIOS_SPEW, "\tCCD_L =   %d\n", mca->nccd_l);
	printk(BIOS_SPEW, "\tWTR_S =   %d\n", mca->nwtr_s);
	printk(BIOS_SPEW, "\tWTR_L =   %d\n", mca->nwtr_l);
	printk(BIOS_SPEW, "\tFAW =     %d\n", mca->nfaw);
	printk(BIOS_SPEW, "\tRCD =     %d\n", mca->nrcd);
	printk(BIOS_SPEW, "\tRP =      %d\n", mca->nrp);
	printk(BIOS_SPEW, "\tRAS =     %d\n", mca->nras);
	printk(BIOS_SPEW, "\tWR =      %d\n", mca->nwr);
	printk(BIOS_SPEW, "\tRRD_S =   %d\n", mca->nrrd_s);
	printk(BIOS_SPEW, "\tRRD_L =   %d\n", mca->nrrd_l);
	printk(BIOS_SPEW, "\tRFC =     %d\n", mca->nrfc);
	printk(BIOS_SPEW, "\tRFC_DLR = %d\n", mca->nrfc_dlr);

	int i;
	for (i = 0; i < 2; i++) {
		if (mca->dimm[i].present) {
			printk(BIOS_SPEW, "\tDIMM%d: %dRx%d ", i, mca->dimm[i].mranks,
			       (mca->dimm[i].width + 1) * 4);

			if (mca->dimm[i].log_ranks != mca->dimm[i].mranks)
				printk(BIOS_SPEW, "%dH 3DS ", mca->dimm[i].log_ranks / mca->dimm[i].mranks);

			printk(BIOS_SPEW, "%dGB\n", mca->dimm[i].size_gb);
		} else {
			printk(BIOS_SPEW, "\tDIMM%d: not installed\n", i);
		}
	}
}

/* TODO: add checks for same ranks configuration for both DIMMs under one MCA */
static inline bool is_proper_dimm(spd_raw_data spd, int slot)
{
	struct dimm_attr_ddr4_st attr;
	if (spd == NULL)
		return false;

	if (spd_decode_ddr4(&attr, spd) != SPD_STATUS_OK) {
		printk(BIOS_ERR, "Malformed SPD for slot %d\n", slot);
		return false;
	}

	if (attr.dram_type != SPD_MEMORY_TYPE_DDR4_SDRAM ||
	    attr.dimm_type != SPD_DDR4_DIMM_TYPE_RDIMM ||
	    !attr.ecc_extension) {
		printk(BIOS_ERR, "Bad DIMM type in slot %d\n", slot);
		return false;
	}

	return true;
}

static void mark_nonfunctional(uint8_t chip, int mcs, int mca)
{
	mem_data[chip].mcs[mcs].mca[mca].functional = false;

	/* Propagate upwards */
	if (!mem_data[chip].mcs[mcs].mca[mca ^ 1].functional) {
		mem_data[chip].mcs[mcs].functional = false;
		if (!mem_data[chip].mcs[mcs ^ 1].functional)
			printk(BIOS_INFO, "No functional MCS left on chip %d\n", chip);
	}
}

static uint64_t find_min_mtb_ftb(uint8_t chip, rdimm_data_t *dimm, int mtb_idx, int ftb_idx)
{
	uint64_t val0 = 0, val1 = 0;

	if (dimm[0].present)
		val0 = mtb_ftb_to_nck(chip, dimm[0].spd[mtb_idx], (int8_t)dimm[0].spd[ftb_idx]);
	if (dimm[1].present)
		val1 = mtb_ftb_to_nck(chip, dimm[1].spd[mtb_idx], (int8_t)dimm[1].spd[ftb_idx]);

	return (val0 < val1) ? val1 : val0;
}

static uint64_t find_min_multi_mtb(uint8_t chip, rdimm_data_t *dimm, int mtb_l, int mtb_h,
				   uint8_t mask, int shift)
{
	uint64_t val0 = 0, val1 = 0;

	if (dimm[0].present)
		val0 = dimm[0].spd[mtb_l] | ((dimm[0].spd[mtb_h] & mask) << shift);
	if (dimm[1].present)
		val1 = dimm[1].spd[mtb_l] | ((dimm[1].spd[mtb_h] & mask) << shift);

	return (val0 < val1) ? mtb_ftb_to_nck(chip, val1, 0) : mtb_ftb_to_nck(chip, val0, 0);
}

/* This is most of step 7 condensed into one function */
static void prepare_cpu_dimm_data(uint8_t chip)
{
	int i, mcs, mca;
	int tckmin = 0x06;		// Platform limit
	unsigned int spd_bus = I2C_BUSES_PER_CPU * chip + SPD_I2C_BUS;

	/*
	 * DIMMs 4-7 are under a different port. This is not the same as bus, but we
	 * need to pass that information to I2C function. As there is no easier way,
	 * use MSB of address and mask it out at the receiving side. This will print
	 * wrong addresses in dump_spd_info(), but that is small price to pay.
	 */
	struct spd_block blk = {
		.addr_map = { DIMM0, DIMM1, DIMM2, DIMM3,
		              DIMM4 | 0x80, DIMM5 | 0x80, DIMM6 | 0x80, DIMM7 | 0x80 },
	};

	get_spd_i2c(spd_bus, &blk);
	dump_spd_info(&blk);

	/*
	 * We need to find the highest common (for all DIMMs and the platform)
	 * supported frequency, meaning we need to compare minimum clock cycle times
	 * and choose the highest value. For the range supported by the platform we
	 * can check MTB only.
	 *
	 * TODO: check if we can have different frequencies across MCSs.
	 */
	for (i = 0; i < CONFIG_DIMM_MAX; i++) {
		if (is_proper_dimm(blk.spd_array[i], i)) {
			mcs = i / DIMMS_PER_MCS;
			mca = (i % DIMMS_PER_MCS) / MCA_PER_MCS;
			int dimm_idx = i % 2;	// (i % DIMMS_PER_MCS) % MCA_PER_MCS


			/* Maximum for 2 DIMMs on one port (channel, MCA) is 2400 MT/s */
			if (tckmin < 0x07 && mem_data[chip].mcs[mcs].mca[mca].functional)
				tckmin = 0x07;

			mem_data[chip].mcs[mcs].functional = true;
			mem_data[chip].mcs[mcs].mca[mca].functional = true;

			rdimm_data_t *dimm = &mem_data[chip].mcs[mcs].mca[mca].dimm[dimm_idx];

			dimm->present = true;
			dimm->spd = blk.spd_array[i];
			/* RCD address is the same as SPD, with one additional bit set */
			dimm->rcd_i2c_addr = blk.addr_map[i] | 0x08;
			/*
			 * SPD fields in spd.h are not compatible with DDR4 and those in
			 * spd_bin.h are just a few of all required.
			 *
			 * TODO: add fields that are lacking to either of those files or
			 * add a file specific to DDR4 SPD.
			 */
			dimm->width = blk.spd_array[i][12] & 7;
			dimm->mranks = ((blk.spd_array[i][12] >> 3) & 0x7) + 1;
			dimm->log_ranks = dimm->mranks * (((blk.spd_array[i][6] >> 4) & 0x7) + 1);
			dimm->density = blk.spd_array[i][4] & 0xF;
			dimm->size_gb = (1 << (dimm->density - 2)) * (2 - dimm->width) *
			                dimm->log_ranks;

			if ((blk.spd_array[i][5] & 0x38) == 0x30)
				die("DIMMs with 18 row address bits are not supported\n");

			if (blk.spd_array[i][18] > tckmin)
				tckmin = blk.spd_array[i][18];
		}
	}

	switch (tckmin) {
	/* For CWL assume 1tCK write preamble */
	case 0x06:
		mem_data[chip].speed = 2666;
		mem_data[chip].cwl = 14;
		break;
	case 0x07:
		mem_data[chip].speed = 2400;
		mem_data[chip].cwl = 12;
		break;
	case 0x08:
		mem_data[chip].speed = 2133;
		mem_data[chip].cwl = 11;
		break;
	case 0x09:
		mem_data[chip].speed = 1866;
		mem_data[chip].cwl = 10;
		break;
	default:
		die("Unsupported tCKmin: %d ps (+/- 125)\n", tckmin * 125);
	}

	/* Now that we know our speed, we can calculate the rest of the data */
	mem_data[chip].nrefi = ns_to_nck(chip, 7800);
	mem_data[chip].nrtp = ps_to_nck(chip, 7500);
	printk(BIOS_SPEW, "Common memory parameters:\n"
	                  "\tspeed =\t%d MT/s\n"
	                  "\tREFI =\t%d clock cycles\n"
	                  "\tCWL =\t%d clock cycles\n"
	                  "\tRTP =\t%d clock cycles\n",
	                  mem_data[chip].speed, mem_data[chip].nrefi,
	                  mem_data[chip].cwl, mem_data[chip].nrtp);

	for (mcs = 0; mcs < MCS_PER_PROC; mcs++) {
		if (!mem_data[chip].mcs[mcs].functional) continue;
		for (mca = 0; mca < MCA_PER_MCS; mca++) {
			if (!mem_data[chip].mcs[mcs].mca[mca].functional) continue;

			rdimm_data_t *dimm = mem_data[chip].mcs[mcs].mca[mca].dimm;
			uint32_t val0, val1, common;
			int min;	/* Minimum compatible with both DIMMs is the bigger value */

			/* CAS Latency */
			val0 = dimm[0].present ? le32_to_cpu(*(uint32_t *)&dimm[0].spd[20]) : -1;
			val1 = dimm[1].present ? le32_to_cpu(*(uint32_t *)&dimm[1].spd[20]) : -1;
			/* Assuming both DIMMs are in low CL range, true for all DDR4 speed bins */
			common = val0 & val1;

			/* tAAmin - minimum CAS latency time */
			min = find_min_mtb_ftb(chip, dimm, 24, 123);
			while (min <= 36 && ((common >> (min - 7)) & 1) == 0)
				min++;

			if (min > 36) {
				/* Maybe just die() instead? */
				printk(BIOS_WARNING, "Cannot find CL supported by all DIMMs under MCS%d, MCA%d."
				       " Marking as nonfunctional.\n", mcs, mca);
				mark_nonfunctional(chip, mcs, mca);
				continue;
			}

			mem_data[chip].mcs[mcs].mca[mca].cl = min;

			/*
			 * There are also minimal values in Table 170 of JEDEC Standard No. 79-4C which
			 * probably should also be honored. Some of them (e.g. RRD) depend on the page
			 * size, which depends on DRAM width. On tested DIMM they are just right - it is
			 * either minimal legal value or rounded up to whole clock cycle. Can we rely on
			 * vendors to put sane values in SPD or do we have to check them for validity?
			 */

			/* Minimum CAS to CAS Delay Time, Same Bank Group */
			mem_data[chip].mcs[mcs].mca[mca].nccd_l = find_min_mtb_ftb(chip, dimm, 40, 117);

			/* Minimum Write to Read Time, Different Bank Group */
			mem_data[chip].mcs[mcs].mca[mca].nwtr_s = find_min_multi_mtb(chip, dimm, 44, 43, 0x0F, 8);

			/* Minimum Write to Read Time, Same Bank Group */
			mem_data[chip].mcs[mcs].mca[mca].nwtr_l = find_min_multi_mtb(chip, dimm, 45, 43, 0xF0, 4);

			/* Minimum Four Activate Window Delay Time */
			mem_data[chip].mcs[mcs].mca[mca].nfaw = find_min_multi_mtb(chip, dimm, 37, 36, 0x0F, 8);

			/* Minimum RAS to CAS Delay Time */
			mem_data[chip].mcs[mcs].mca[mca].nrcd = find_min_mtb_ftb(chip, dimm, 25, 122);

			/* Minimum Row Precharge Delay Time */
			mem_data[chip].mcs[mcs].mca[mca].nrp = find_min_mtb_ftb(chip, dimm, 26, 121);

			/* Minimum Active to Precharge Delay Time */
			mem_data[chip].mcs[mcs].mca[mca].nras = find_min_multi_mtb(chip, dimm, 28, 27, 0x0F, 8);

			/* Minimum Write Recovery Time */
			mem_data[chip].mcs[mcs].mca[mca].nwr = find_min_multi_mtb(chip, dimm, 42, 41, 0x0F, 8);

			/* Minimum Activate to Activate Delay Time, Different Bank Group */
			mem_data[chip].mcs[mcs].mca[mca].nrrd_s = find_min_mtb_ftb(chip, dimm, 38, 119);

			/* Minimum Activate to Activate Delay Time, Same Bank Group */
			mem_data[chip].mcs[mcs].mca[mca].nrrd_l = find_min_mtb_ftb(chip, dimm, 39, 118);

			/* Minimum Refresh Recovery Delay Time */
			/* Assuming no fine refresh mode. */
			mem_data[chip].mcs[mcs].mca[mca].nrfc = find_min_multi_mtb(chip, dimm, 30, 31, 0xFF, 8);

			/* Minimum Refresh Recovery Delay Time for Different Logical Rank (3DS only) */
			/*
			 * This one is set per MCA, but it depends on DRAM density, which can be
			 * mixed between DIMMs under the same channel. We need to choose the bigger
			 * minimum time, which corresponds to higher density.
			 *
			 * Assuming no fine refresh mode.
			 */
			val0 = dimm[0].present ? dimm[0].spd[4] & 0xF : 0;
			val1 = dimm[1].present ? dimm[1].spd[4] & 0xF : 0;
			min = (val0 < val1) ? val1 : val0;

			switch (min) {
			case 0x4:
				mem_data[chip].mcs[mcs].mca[mca].nrfc_dlr = ns_to_nck(chip, 90);
				break;
			case 0x5:
				mem_data[chip].mcs[mcs].mca[mca].nrfc_dlr = ns_to_nck(chip, 120);
				break;
			case 0x6:
				mem_data[chip].mcs[mcs].mca[mca].nrfc_dlr = ns_to_nck(chip, 185);
				break;
			default:
				die("Unsupported DRAM density\n");
			}

			printk(BIOS_SPEW, "MCS%d, MCA%d times (in clock cycles):\n", mcs, mca);
			dump_mca_data(&mem_data[chip].mcs[mcs].mca[mca]);
		}
	}
}

/* This is most of step 7 condensed into one function */
static void prepare_dimm_data(uint8_t chips)
{
	bool have_dimms = false;

	uint8_t chip;

	for (chip = 0; chip < MAX_CHIPS; chip++) {
		int mcs;

		if (chips & (1 << chip))
			prepare_cpu_dimm_data(chip);

		for (mcs = 0; mcs < MCS_PER_PROC; mcs++)
			have_dimms |= mem_data[chip].mcs[mcs].functional;
	}

	/*
	 * There is one (?) MCBIST per CPU. Fail if there are no supported DIMMs
	 * connected, otherwise assume it is functional. There is no reason to redo
	 * this test in the rest of isteps.
	 */
	if (!have_dimms)
		die("No DIMMs detected, aborting\n");
}

static void build_mvpds(uint8_t chips)
{
	uint8_t chip;

	printk(BIOS_EMERG, "Building MVPDs...\n");

	/* Calling mvpd_get_available_cores() triggers building and caching of MVPD */
	for (chip = 0; chip < MAX_CHIPS; ++chip) {
		if (chips & (1 << chip))
			(void)mvpd_get_available_cores(chip);
	}
}

void main(void)
{
	uint8_t chips;

	struct pci_info pci_info[MAX_CHIPS] = { 0 };

	init_timer();

	timestamp_add_now(TS_START_ROMSTAGE);

	console_init();

	if (ipmi_premem_init(CONFIG_BMC_BT_BASE, 0) != CB_SUCCESS)
		die("Failed to initialize IPMI\n");

	/*
	 * Two minutes to load.
	 * Not handling return code, because the function itself prints log messages
	 * and its failure is not a critical error.
	 */
	(void)ipmi_init_and_start_bmc_wdt(CONFIG_BMC_BT_BASE, 120, TIMEOUT_HARD_RESET);

	printk(BIOS_EMERG, "Initializing FSI...\n");
	fsi_init();
	chips = fsi_get_present_chips();
	printk(BIOS_EMERG, "Initialized FSI (chips mask: 0x%02X)\n", chips);

	build_mvpds(chips);

	istep_8_1(chips);
	istep_8_2(chips);
	istep_8_3(chips);
	istep_8_4(chips);
	istep_8_9(chips);
	istep_8_10(chips);
	istep_8_11(chips);

	istep_9_2(chips);
	istep_9_4(chips);
	istep_9_6(chips);
	istep_9_7(chips);

	istep_10_1(chips);
	istep_10_6(chips);
	istep_10_10(chips, pci_info);
	istep_10_12(chips);
	istep_10_13(chips);

	timestamp_add_now(TS_BEFORE_INITRAM);

	vpd_pnor_main();
	prepare_dimm_data(chips);

	report_istep(13, 1);	// no-op
	istep_13_2(chips);
	istep_13_3(chips);
	istep_13_4(chips);
	report_istep(13, 5);	// no-op
	istep_13_6(chips);
	report_istep(13, 7);	// no-op
	istep_13_8(chips);
	istep_13_9(chips);
	istep_13_10(chips);
	istep_13_11(chips);
	report_istep(13, 12);	// optional, not yet implemented
	istep_13_13(chips);

	istep_14_1(chips);
	istep_14_2(chips);
	istep_14_3(chips, pci_info);
	report_istep(14, 4);	// no-op
	istep_14_5(chips);

	timestamp_add_now(TS_AFTER_INITRAM);

	/* Test if SCOM still works. Maybe should check also indirect access? */
	printk(BIOS_DEBUG, "0xF000F = %llx\n", read_scom(0, 0xF000F));

	/*
	 * Halt to give a chance to inspect FIRs, otherwise checkstops from
	 * ramstage may cover up the failure in romstage.
	 */
	if (read_scom(0, 0xF000F) == 0xFFFFFFFFFFFFFFFF)
		die("SCOM stopped working, check FIRs, halting now\n");

	cbmem_initialize_empty();
	run_ramstage();
}

/* Stores global mem_data variable into cbmem for future use by ramstage */
static void store_mem_data(int is_recovery)
{
	const struct cbmem_entry *entry;
	uint8_t *data;
	int dimm_i;

	(void)is_recovery; /* unused */

	/* Layout: mem_data itself then SPD data of each dimm which has it */
	entry = cbmem_entry_add(CBMEM_ID_MEMINFO, sizeof(mem_data) +
				MAX_CHIPS * DIMMS_PER_PROC * CONFIG_DIMM_SPD_SIZE);
	if (entry == NULL)
		die("Failed to add mem_data entry to CBMEM in romstage!");

	data = cbmem_entry_start(entry);

	memcpy(data, &mem_data, sizeof(mem_data));
	data += sizeof(mem_data);

	for (dimm_i = 0; dimm_i < MAX_CHIPS * DIMMS_PER_PROC; dimm_i++) {
		int chip = dimm_i / DIMMS_PER_PROC;
		int mcs = (dimm_i % DIMMS_PER_PROC) / DIMMS_PER_MCS;
		int mca = (dimm_i % DIMMS_PER_MCS) / DIMMS_PER_MCA;
		int dimm = dimm_i % DIMMS_PER_MCA;

		rdimm_data_t *dimm_data = &mem_data[chip].mcs[mcs].mca[mca].dimm[dimm];
		if (dimm_data->spd == NULL)
			continue;

		memcpy(data, dimm_data->spd, CONFIG_DIMM_SPD_SIZE);
		data += CONFIG_DIMM_SPD_SIZE;
	}
}
CBMEM_CREATION_HOOK(store_mem_data);
