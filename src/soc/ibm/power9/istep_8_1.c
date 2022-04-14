/* SPDX-License-Identifier: GPL-2.0-only */

#include <arch/io.h>
#include <stdint.h>
#include <string.h>
#include <console/console.h>
#include <cpu/power/istep_8.h>
#include <cpu/power/mvpd.h>
#include <cpu/power/proc.h>
#include <cpu/power/scom.h>

#include "fsi.h"
#include "scratch.h"

/*
 * Legend for constant names:
 * - *_FSI is a CFAM address (in 4 byte words, that's how it is in Hostboot)
 * - *_FSI_BYTE is an FSI address
 */

/* Used to read SBE Boot Side from processor */
const uint64_t SBE_BOOT_SELECT_MASK = 0x0000400000000000;

static void compute_chip_gards(uint8_t chip,
			       uint8_t *eq_gard, uint32_t *ec_gard)
{
	const uint64_t cores = mvpd_get_available_cores(chip);

	*eq_gard = 0xFF;
	*ec_gard = 0xFFFFFFFF;

	for (int quad = 0; quad < MAX_QUADS_PER_CHIP; quad++) {
		if (IS_EQ_FUNCTIONAL(quad, cores))
			*eq_gard &= ~(0x80 >> quad);
	}

	for (int core = 0; core < MAX_CORES_PER_CHIP; core++) {
		if (IS_EC_FUNCTIONAL(core, cores))
			*ec_gard &= ~(0x80000000 >> core);
	}

	/* Shift the first meaningful bit to LSB position */
	*eq_gard >>= 2;
	*ec_gard >>= 8;
}

static void setup_sbe_config(uint8_t chip)
{
	/* These aren't defined in scratch.h because they are used only here and
	 * this allows for much shorter names. */
	enum {
		/* SCRATCH_REGISTER_1 */
		EQ_GARD_START             = 0,
		EQ_GARD_LEN               = 6,
		EC_GARD_START             = 8,
		EC_GARD_LEN               = 24,

		/* SCRATCH_REGISTER_2 */
		I2C_BUS_DIV_REF_START     = 0,
		I2C_BUS_DIV_REF_LEN       = 16,
		OPTICS_CONFIG_MODE_OBUS0  = 16,
		OPTICS_CONFIG_MODE_OBUS1  = 17,
		OPTICS_CONFIG_MODE_OBUS2  = 18,
		OPTICS_CONFIG_MODE_OBUS3  = 19,
		MC_PLL_BUCKET_START       = 21,
		MC_PLL_BUCKET_LEN         = 3,
		OB0_PLL_BUCKET_START      = 24,
		OB0_PLL_BUCKET_LEN        = 2,
		OB1_PLL_BUCKET_START      = 26,
		OB1_PLL_BUCKET_LEN        = 2,
		OB2_PLL_BUCKET_START      = 28,
		OB2_PLL_BUCKET_LEN        = 2,
		OB3_PLL_BUCKET_START      = 30,
		OB3_PLL_BUCKET_LEN        = 2,

		/* SCRATCH_REGISTER_3 */
		BOOT_FLAGS_START          = 0,
		BOOT_FLAGS_LEN            = 32,
		RISK_LEVEL_START          = 28,
		RISK_LEVEL_LEN            = 4,

		/* SCRATCH_REGISTER_4 */
		BOOT_FREQ_MULT_START      = 0,
		BOOT_FREQ_MULT_LEN        = 16,
		CP_FILTER_BYPASS          = 16,
		SS_FILTER_BYPASS          = 17,
		IO_FILTER_BYPASS          = 18,
		DPLL_BYPASS               = 19,
		NEST_MEM_X_O_PCI_BYPASS   = 20,
		OBUS_RATIO_VALUE          = 21,
		NEST_PLL_BUCKET_START     = 29,
		NEST_PLL_BUCKET_LEN       = 3,

		/* SCRATCH_REGISTER_5 */
		PLL_MUX_START             = 12,
		PLL_MUX_LEN               = 20,
		CC_IPL                    = 0,
		INIT_ALL_CORES            = 1,
		RISK_LEVEL_BIT_DEPRECATED = 2,
		DISABLE_HBBL_VECTORS      = 3,
		MC_SYNC_MODE              = 4,
		SLOW_PCI_REF_CLOCK        = 5,

		/* SCRATCH_REGISTER_6 */
		SMF_CONFIG                     = 16,
		PROC_EFF_FABRIC_GROUP_ID_START = 17,
		PROC_EFF_FABRIC_GROUP_ID_LEN   = 3,
		PROC_EFF_FABRIC_CHIP_ID_START  = 20,
		PROC_EFF_FABRIC_CHIP_ID_LEN    = 3,
		PUMP_CHIP_IS_GROUP             = 23,
		SLAVE_CHIP_SBE                 = 24,
		PROC_FABRIC_GROUP_ID_START     = 26,
		PROC_FABRIC_GROUP_ID_LEN       = 3,
		PROC_FABRIC_CHIP_ID_START      = 29,
		PROC_FABRIC_CHIP_ID_LEN        = 3,
		PROC_MEM_TO_USE_START          = 1,
		PROC_MEM_TO_USE_LEN            = 6,
	};

	uint64_t scratch;

	uint32_t boot_flags;
	uint8_t risk_level;

	uint8_t eq_gard;
	uint32_t ec_gard;

	compute_chip_gards(chip, &eq_gard, &ec_gard);

	/* SCRATCH_REGISTER_1 */

	scratch = PPC_SHIFT(read_cfam(chip, MBOX_SCRATCH_REG1_FSI), 31);

	/* ATTR_EQ_GARD (computed at runtime) */
	PPC_INSERT(scratch, eq_gard, EQ_GARD_START, EQ_GARD_LEN);
	/* ATTR_EC_GARD (computed at runtime)*/
	PPC_INSERT(scratch, ec_gard, EC_GARD_START, EC_GARD_LEN);

	write_cfam(chip, MBOX_SCRATCH_REG1_FSI, scratch >> 32);

	/* SCRATCH_REGISTER_2 */

	scratch = PPC_SHIFT(read_cfam(chip, MBOX_SCRATCH_REG1_FSI + 1), 31);

	/* ATTR_I2C_BUS_DIV_REF (default, talos.xml) */
	PPC_INSERT(scratch, 0x0003, I2C_BUS_DIV_REF_START, I2C_BUS_DIV_REF_LEN);
	/* ATTR_MC_PLL_BUCKET (seems to not by relevant for Nimbus) */
	PPC_INSERT(scratch, 0x00, MC_PLL_BUCKET_START, MC_PLL_BUCKET_LEN);

	/* TODO: set OPTICS_CONFIG_MODE_OBUS[0-3] bits when adding support for OBUS. */
	/* TODO: set OB[0-3]_PLL_BUCKET bits when adding support for OBUS,
	 *       see getObusPllBucket() in Hostboot for values of ATTR_OB*_PLL_BUCKET */

	write_cfam(chip, MBOX_SCRATCH_REG1_FSI + 1, scratch >> 32);

	/* SCRATCH_REGISTER_3 */

	scratch = PPC_SHIFT(read_cfam(chip, MBOX_SCRATCH_REG1_FSI + 2), 31);

	boot_flags = (read_rscom(0, MBOX_SCRATCH_REG1 + 2) >> 32);
	risk_level = (get_dd() < 0x23 ? 0 : 4);

	/* Note that the two fields overlap (boot flags include risk level), so
	 * order in which they are set is important. */

	/* ATTR_BOOT_FLAGS (computed) */
	PPC_INSERT(scratch, boot_flags, BOOT_FLAGS_START, BOOT_FLAGS_LEN);
	/* ATTR_RISK_LEVEL (computed) */
	PPC_INSERT(scratch, risk_level, RISK_LEVEL_START, RISK_LEVEL_LEN);

	write_cfam(chip, MBOX_SCRATCH_REG1_FSI + 2, scratch >> 32);

	/* SCRATCH_REGISTER_4 */

	scratch = PPC_SHIFT(read_cfam(chip, MBOX_SCRATCH_REG1_FSI + 3), 31);

	/* ATTR_BOOT_FREQ_MULT (talos.xml) */
	PPC_INSERT(scratch, 96, BOOT_FREQ_MULT_START, BOOT_FREQ_MULT_LEN);
	/* ATTR_NEST_PLL_BUCKET (index of nest frequency (1866 MHz) in
	 * ATTR_NEST_PLL_FREQ_LIST array base 1, see setFrequencyAttributes() in
	 * Hostboot) */
	PPC_INSERT(scratch, 2, NEST_PLL_BUCKET_START, NEST_PLL_BUCKET_LEN);

	/* ATTR_CP_FILTER_BYPASS (default, talos.xml) */
	PPC_INSERT(scratch, 0, CP_FILTER_BYPASS, 1);
	/* ATTR_SS_FILTER_BYPASS (default, talos.xml) */
	PPC_INSERT(scratch, 0, SS_FILTER_BYPASS, 1);
	/* ATTR_IO_FILTER_BYPASS (default, talos.xml) */
	PPC_INSERT(scratch, 0, IO_FILTER_BYPASS, 1);
	/* ATTR_DPLL_BYPASS (default, talos.xml) */
	PPC_INSERT(scratch, 0, DPLL_BYPASS, 1);
	/* ATTR_NEST_MEM_X_O_PCI_BYPASS (default, talos.xml) */
	PPC_INSERT(scratch, 0, NEST_MEM_X_O_PCI_BYPASS, 1);

	/* ATTR_OBUS_RATIO_VALUE (empty default in talos.xml) */
	PPC_INSERT(scratch, 0, OBUS_RATIO_VALUE, 1);

	write_cfam(chip, MBOX_SCRATCH_REG1_FSI + 3, scratch >> 32);

	/* SCRATCH_REGISTER_5 */

	scratch = PPC_SHIFT(read_cfam(chip, MBOX_SCRATCH_REG1_FSI + 4), 31);

	/* ATTR_SYSTEM_IPL_PHASE (default, talos.xml) == HB_IPL, not CACHE_CONTAINED
	 * XXX: but we're not Hostboot and in ROM stage, so set this bit to 1? */
	PPC_INSERT(scratch, 0, CC_IPL, 1);
	/* ATTR_SYS_FORCE_ALL_CORES (talos.xml) */
	PPC_INSERT(scratch, 0, INIT_ALL_CORES, 1);
	/* Risk level flag is deprecated here, moved to SCRATCH_REG_3 */
	PPC_INSERT(scratch, 0, RISK_LEVEL_BIT_DEPRECATED, 1);
	/* ATTR_DISABLE_HBBL_VECTORS (default, talos.xml) */
	PPC_INSERT(scratch, 0, DISABLE_HBBL_VECTORS, 1);
	/* Hostboot reads it from SBE, but we assume it's 0 in p9n_mca_scom() */
	PPC_INSERT(scratch, 0, MC_SYNC_MODE, 1);
	/* ATTR_DD1_SLOW_PCI_REF_CLOCK (we're not DD1, but Hostboot sets this bit) */
	PPC_INSERT(scratch, 1, SLOW_PCI_REF_CLOCK, 1);

	/* ATTR_CLOCK_PLL_MUX (talos.xml) */
	PPC_INSERT(scratch, 0x80030, PLL_MUX_START, PLL_MUX_LEN);

	write_cfam(chip, MBOX_SCRATCH_REG1_FSI + 4, scratch >> 32);

	/* SCRATCH_REGISTER_6 */

	scratch = PPC_SHIFT(read_cfam(chip, MBOX_SCRATCH_REG1_FSI + 5), 31);

	/* ATTR_PROC_SBE_MASTER_CHIP is always zero here */
	PPC_INSERT(scratch, 1, SLAVE_CHIP_SBE, 1);
	/* ATTR_SMF_CONFIG */
	PPC_INSERT(scratch, 0, SMF_CONFIG, 1);
	/* ATTR_PROC_FABRIC_PUMP_MODE (talos.xml) */
	PPC_INSERT(scratch, 1, PUMP_CHIP_IS_GROUP, 1);

	/* ATTR_PROC_FABRIC_GROUP_ID */
	PPC_INSERT(scratch, chip, PROC_FABRIC_GROUP_ID_START, PROC_FABRIC_GROUP_ID_LEN);
	/* ATTR_PROC_FABRIC_CHIP_ID */
	PPC_INSERT(scratch, 0, PROC_FABRIC_CHIP_ID_START, PROC_FABRIC_CHIP_ID_LEN);

	/* ATTR_PROC_EFF_FABRIC_GROUP_ID */
	PPC_INSERT(scratch, chip, PROC_EFF_FABRIC_GROUP_ID_START, PROC_EFF_FABRIC_GROUP_ID_LEN);
	/* ATTR_PROC_EFF_FABRIC_CHIP_ID */
	PPC_INSERT(scratch, 0, PROC_EFF_FABRIC_CHIP_ID_START, PROC_EFF_FABRIC_CHIP_ID_LEN);

	/* Not documented what this is */
	scratch |= PPC_BIT(0);

	/* ATTR_PROC_MEM_TO_USE (talos.xml; each CPU uses its own memory) */
	PPC_INSERT(scratch, chip << 3, PROC_MEM_TO_USE_START, PROC_MEM_TO_USE_LEN);

	write_cfam(chip, MBOX_SCRATCH_REG1_FSI + 5, scratch >> 32);

	/* SCRATCH_REGISTER_7 is left as is (it's related to DRTM payload) */

	/* SCRATCH_REGISTER_8 */

	scratch = PPC_SHIFT(read_cfam(chip, MBOX_SCRATCH_REG1_FSI + 7), 31);

	/* Indicate validity of SCRATCH_REGISTER_[1-6] */
	scratch |= PPC_BITMASK(0, 5);

	write_cfam(chip, MBOX_SCRATCH_REG1_FSI + 7, scratch >> 32);
}

static int get_master_sbe_boot_seeprom(void)
{
	enum { PERV_SB_CS_SCOM = 0x00050008 };
	return (read_rscom(0, PERV_SB_CS_SCOM) & SBE_BOOT_SELECT_MASK) ? 1 : 0;
}

static void set_sbe_boot_seeprom(uint8_t chip, int seeprom_side)
{
	enum { PERV_SB_CS_FSI_BYTE = 0x00002820 };

	const uint32_t sbe_boot_select_mask = SBE_BOOT_SELECT_MASK >> 32;

	uint32_t sb_cs = read_fsi(chip, PERV_SB_CS_FSI_BYTE);

	if (seeprom_side == 0)
		sb_cs &= ~sbe_boot_select_mask;
	else
		sb_cs |= sbe_boot_select_mask;

	write_fsi(chip, PERV_SB_CS_FSI_BYTE, sb_cs);
}

void istep_8_1(uint8_t chips)
{
	int boot_seeprom_side;

	printk(BIOS_EMERG, "starting istep 8.1\n");
	report_istep(8,1);

	boot_seeprom_side = get_master_sbe_boot_seeprom();

	/* Skipping master chip */
	for (uint8_t chip = 1; chip < MAX_CHIPS; chip++) {
		if (chips & (1 << chip)) {
			setup_sbe_config(chip);
			set_sbe_boot_seeprom(chip, boot_seeprom_side);
		}
	}

	printk(BIOS_EMERG, "ending istep 8.1\n");
}
