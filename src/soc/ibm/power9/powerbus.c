/* SPDX-License-Identifier: GPL-2.0-only */

#include <cpu/power/powerbus.h>

#include <cpu/power/mvpd.h>
#include <cpu/power/scom.h>
#include <console/console.h>
#include <stdint.h>

#define EPSILON_MAX_VALUE 0xFFFFFFFF

#define MBOX_SCRATCH_REG1 0x00050038
#define MBOX_SCRATCH_REG6_GROUP_PUMP_MODE 23

#define EPS_GUARDBAND 20

/* From src/import/chips/p9/procedures/hwp/nest/p9_fbc_eff_config.C */
/* LE epsilon (2 chips per-group) */
static const uint32_t EPSILON_R_T0_LE[] = {    7,    7,    8,    8,   10,   22 };
static const uint32_t EPSILON_R_T1_LE[] = {    7,    7,    8,    8,   10,   22 };
static const uint32_t EPSILON_R_T2_LE[] = {   67,   69,   71,   74,   79,  103 };
static const uint32_t EPSILON_W_T0_LE[] = {    0,    0,    0,    0,    0,    5 };
static const uint32_t EPSILON_W_T1_LE[] = {   15,   16,   17,   19,   21,   33 };

/* See get_first_valid_pdV_pbFreq() in Hostboot */

static bool read_voltage_data(struct powerbus_cfg *cfg)
{
	int i = 0;
	const struct voltage_kwd *voltage = NULL;

	/* ATTR_FREQ_PB_MHZ, equal to the first non-zero PowerBus frequency */
	uint32_t pb_freq = 0;
	/* ATTR_FREQ_CORE_CEILING_MHZ, equal to the minimum of turbo frequencies */
	uint32_t freq_ceiling = 0;
	/* ATTR_FREQ_CORE_FLOOR_MHZ, equal to the maximum of powersave frequencies */
	uint32_t freq_floor = 0;

	/* Using LRP0 because frequencies are the same in all LRP records */
	/* TODO: don't hard-code chip if values are not the same among them */
	voltage = mvpd_get_voltage_data(/*chip=*/0, /*lrp=*/0);

	for (i = 0; i < VOLTAGE_BUCKET_COUNT; ++i) {
		const struct voltage_bucket_data *bucket = &voltage->buckets[i];
		if (bucket->id == 0)
			continue;

		if (pb_freq == 0 && bucket->powerbus.freq != 0)
			pb_freq = bucket->powerbus.freq;

		if (bucket->powersave.freq != 0 &&
		    (freq_floor == 0 || bucket->powersave.freq > freq_floor)) {
			freq_floor = bucket->powersave.freq;
		}

		if (bucket->turbo.freq != 0 &&
		    (freq_ceiling == 0 || bucket->turbo.freq < freq_ceiling)) {
			freq_ceiling = bucket->turbo.freq;
		}
	}

	cfg->fabric_freq = pb_freq;
	cfg->freq_core_floor = freq_floor;
	cfg->freq_core_ceiling = freq_ceiling;

	return true;
}

static bool calculate_frequencies(struct powerbus_cfg *cfg)
{
	const uint32_t pb_freq = cfg->fabric_freq;
	const uint32_t freq_floor = cfg->freq_core_floor;
	const uint32_t freq_ceiling = cfg->freq_core_ceiling;

	enum FABRIC_CORE_FLOOR_RATIO floor_ratio;
	enum FABRIC_CORE_CEILING_RATIO ceiling_ratio;

	/* breakpoint ratio: core floor 4.0, pb 2.0 (cache floor :: pb = 8/8) */
	if (freq_floor >= (2 * pb_freq)) {
		floor_ratio = FABRIC_CORE_FLOOR_RATIO_RATIO_8_8;
	/* breakpoint ratio: core floor 3.5, pb 2.0 (cache floor :: pb = 7/8) */
	} else if ((4 * freq_floor) >= (7 * pb_freq)) {
		floor_ratio = FABRIC_CORE_FLOOR_RATIO_RATIO_7_8;
	/* breakpoint ratio: core floor 3.0, pb 2.0 (cache floor :: pb = 6/8) */
	} else if ((2 * freq_floor) >= (3 * pb_freq)) {
		floor_ratio = FABRIC_CORE_FLOOR_RATIO_RATIO_6_8;
	/* breakpoint ratio: core floor 2.5, pb 2.0 (cache floor :: pb = 5/8) */
	} else if ((4 * freq_floor) >= (5 * pb_freq)) {
		floor_ratio = FABRIC_CORE_FLOOR_RATIO_RATIO_5_8;
	/* breakpoint ratio: core floor 2.0, pb 2.0 (cache floor :: pb = 4/8) */
	} else if (freq_floor >= pb_freq) {
		floor_ratio = FABRIC_CORE_FLOOR_RATIO_RATIO_4_8;
	/* breakpoint ratio: core floor 1.0, pb 2.0 (cache floor :: pb = 2/8) */
	} else if ((2 * freq_floor) >= pb_freq) {
		floor_ratio = FABRIC_CORE_FLOOR_RATIO_RATIO_2_8;
	} else {
		printk(BIOS_ERR, "Unsupported core ceiling/PB frequency ratio = (%d/%d)\n",
			freq_floor, pb_freq);
		return false;
	}

	/* breakpoint ratio: core ceiling 4.0, pb 2.0 (cache ceiling :: pb = 8/8) */
	if (freq_ceiling >= (2 * pb_freq)) {
		ceiling_ratio = FABRIC_CORE_CEILING_RATIO_RATIO_8_8;
	/* breakpoint ratio: core ceiling 3.5, pb 2.0 (cache ceiling :: pb = 7/8) */
	} else if ((4 * freq_ceiling) >= (7 * pb_freq)) {
		ceiling_ratio = FABRIC_CORE_CEILING_RATIO_RATIO_7_8;
	/* breakpoint ratio: core ceiling 3.0, pb 2.0 (cache ceiling :: pb = 6/8) */
	} else if ((2 * freq_ceiling) >= (3 * pb_freq)) {
		ceiling_ratio = FABRIC_CORE_CEILING_RATIO_RATIO_6_8;
	/* breakpoint ratio: core ceiling 2.5, pb 2.0 (cache ceiling :: pb = 5/8) */
	} else if ((4 * freq_ceiling) >= (5 * pb_freq)) {
		ceiling_ratio = FABRIC_CORE_CEILING_RATIO_RATIO_5_8;
	/* breakpoint ratio: core ceiling 2.0, pb 2.0 (cache ceiling :: pb = 4/8) */
	} else if (freq_ceiling >= pb_freq) {
		ceiling_ratio = FABRIC_CORE_CEILING_RATIO_RATIO_4_8;
	/* breakpoint ratio: core ceiling 1.0, pb 2.0 (cache ceiling :: pb = 2/8) */
	} else if ((2 * freq_ceiling) >= pb_freq) {
		ceiling_ratio = FABRIC_CORE_CEILING_RATIO_RATIO_2_8;
	} else {
		printk(BIOS_ERR, "Unsupported core ceiling/PB frequency ratio = (%d/%d)\n",
			freq_ceiling, pb_freq);
		return false;
	}

	cfg->core_floor_ratio = floor_ratio;
	cfg->core_ceiling_ratio = ceiling_ratio;
	return true;
}

static void config_guardband_epsilon(uint8_t gb_percentage, uint32_t *target_value)
{
	uint32_t delta = (*target_value * gb_percentage) / 100;
	delta += ((*target_value * gb_percentage) % 100) ? 1 : 0;

	/* Clamp to maximum value if necessary */
	if (delta > (EPSILON_MAX_VALUE - *target_value)) {
		printk(BIOS_DEBUG, "Guardband application generated out-of-range target value,"
		       " clamping to maximum value!\n");
		*target_value = EPSILON_MAX_VALUE;
	} else {
		*target_value += delta;
	}
}

static void dump_epsilons(struct powerbus_cfg *cfg)
{
	uint32_t i;

	for (i = 0; i < NUM_EPSILON_READ_TIERS; i++)
		printk(BIOS_DEBUG, " R_T[%d] = %d\n", i, cfg->eps_r[i]);

	for (i = 0; i < NUM_EPSILON_WRITE_TIERS; i++)
		printk(BIOS_DEBUG, " W_T[%d] = %d\n", i, cfg->eps_w[i]);
}

static void calculate_epsilons(struct powerbus_cfg *cfg)
{
	const enum FABRIC_CORE_FLOOR_RATIO floor_ratio = cfg->core_floor_ratio;
	const enum FABRIC_CORE_CEILING_RATIO ceiling_ratio = cfg->core_ceiling_ratio;
	const uint32_t pb_freq = cfg->fabric_freq;
	const uint32_t freq_ceiling = cfg->freq_core_ceiling;

	uint32_t *eps_r = cfg->eps_r;
	uint32_t *eps_w = cfg->eps_w;

	uint32_t i;

	uint64_t scratch_reg6 = read_scom(MBOX_SCRATCH_REG1 + 5);
	/* ATTR_PROC_FABRIC_PUMP_MODE, it's either node or group pump mode */
	bool node_pump_mode = !(scratch_reg6 & PPC_BIT(MBOX_SCRATCH_REG6_GROUP_PUMP_MODE));

	/* Assuming that ATTR_PROC_EPS_TABLE_TYPE = EPS_TYPE_LE in talos.xml is always correct */

	eps_r[0] = EPSILON_R_T0_LE[floor_ratio];

	if (node_pump_mode)
		eps_r[1] = EPSILON_R_T1_LE[floor_ratio];
	else
		eps_r[1] = EPSILON_R_T0_LE[floor_ratio];

	eps_r[2] = EPSILON_R_T2_LE[floor_ratio];

	eps_w[0] = EPSILON_W_T0_LE[floor_ratio];
	eps_w[1] = EPSILON_W_T1_LE[floor_ratio];

	/* Dump base epsilon values */
	printk(BIOS_DEBUG, "Base epsilon values read from table:\n");
	dump_epsilons(cfg);

	/* Scale base epsilon values if core is running 2x nest frequency */
	if (ceiling_ratio == FABRIC_CORE_CEILING_RATIO_RATIO_8_8) {
		uint8_t scale_percentage = 100 * freq_ceiling / (2 * pb_freq);
		if (scale_percentage < 100)
			die("scale_percentage is too small!");
		scale_percentage -= 100;

		printk(BIOS_DEBUG, "Scaling based on ceiling frequency\n");

		for (i = 0; i < NUM_EPSILON_READ_TIERS; i++)
			config_guardband_epsilon(scale_percentage, &eps_r[i]);

		for (i = 0; i < NUM_EPSILON_WRITE_TIERS; i++)
			config_guardband_epsilon(scale_percentage, &eps_w[i]);
	}

	for (i = 0; i < NUM_EPSILON_READ_TIERS; i++)
		config_guardband_epsilon(EPS_GUARDBAND, &eps_r[i]);

	for (i = 0; i < NUM_EPSILON_WRITE_TIERS; i++)
		config_guardband_epsilon(EPS_GUARDBAND, &eps_w[i]);

	/* Dump final epsilon values */
	printk(BIOS_DEBUG, "Scaled epsilon values based on %s%d percent guardband:\n",
	       (EPS_GUARDBAND >= 0 ? "+" : "-"), EPS_GUARDBAND);
	dump_epsilons(cfg);

	/*
	 * Check relationship of epsilon counters:
	 *   read tier values are strictly increasing
	 *   write tier values are strictly increasing
	 */
	if (eps_r[0] > eps_r[1] || eps_r[1] > eps_r[2] || eps_w[0] > eps_w[1])
		printk(BIOS_WARNING, "Invalid relationship between base epsilon values\n");
}

const struct powerbus_cfg *powerbus_cfg(void)
{
	static struct powerbus_cfg cfg;

	static bool init_done;
	if (init_done)
		return &cfg;

	if (!read_voltage_data(&cfg))
		die("Failed to read voltage data");

	if (!calculate_frequencies(&cfg))
		die("Incorrect core or PowerBus frequency");

	calculate_epsilons(&cfg);

	init_done = true;
	return &cfg;
}
