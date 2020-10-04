/* SPDX-License-Identifier: GPL-2.0-only */

#include <console/console.h>
#include <console/usb.h>
#include <cf9_reset.h>
#include <string.h>
#include <device/device.h>
#include <device/pci_ops.h>
#include <arch/cpu.h>
#include <cbmem.h>
#include <cbfs.h>
#include <ip_checksum.h>
#include <pc80/mc146818rtc.h>
#include <device/pci_def.h>
#include <lib.h>
#include <mrc_cache.h>
#include <stddef.h>
#include <stdint.h>
#include <timestamp.h>
#include "raminit.h"
#include "pei_data.h"
#include "sandybridge.h"
#include "chip.h"
#include <security/vboot/vboot_common.h>
#include <southbridge/intel/bd82x6x/pch.h>

/* Management Engine is in the southbridge */
#include <southbridge/intel/bd82x6x/me.h>

/*
 * MRC scrambler seed offsets should be reserved in
 * mainboard cmos.layout and not covered by checksum.
 */
#if CONFIG(USE_OPTION_TABLE)
#include "option_table.h"
#define CMOS_OFFSET_MRC_SEED     (CMOS_VSTART_mrc_scrambler_seed     >> 3)
#define CMOS_OFFSET_MRC_SEED_S3  (CMOS_VSTART_mrc_scrambler_seed_s3  >> 3)
#define CMOS_OFFSET_MRC_SEED_CHK (CMOS_VSTART_mrc_scrambler_seed_chk >> 3)
#else
#define CMOS_OFFSET_MRC_SEED     152
#define CMOS_OFFSET_MRC_SEED_S3  156
#define CMOS_OFFSET_MRC_SEED_CHK 160
#endif

#define MRC_CACHE_VERSION 0

void save_mrc_data(struct pei_data *pei_data)
{
	u16 c1, c2, checksum;

	/* Save the MRC S3 restore data to cbmem */
	mrc_cache_stash_data(MRC_TRAINING_DATA, MRC_CACHE_VERSION, pei_data->mrc_output,
			pei_data->mrc_output_len);

	/* Save the MRC seed values to CMOS */
	cmos_write32(pei_data->scrambler_seed, CMOS_OFFSET_MRC_SEED);
	printk(BIOS_DEBUG, "Save scrambler seed    0x%08x to CMOS 0x%02x\n",
	       pei_data->scrambler_seed, CMOS_OFFSET_MRC_SEED);

	cmos_write32(pei_data->scrambler_seed_s3, CMOS_OFFSET_MRC_SEED_S3);
	printk(BIOS_DEBUG, "Save s3 scrambler seed 0x%08x to CMOS 0x%02x\n",
	       pei_data->scrambler_seed_s3, CMOS_OFFSET_MRC_SEED_S3);

	/* Save a simple checksum of the seed values */
	c1 = compute_ip_checksum((u8 *)&pei_data->scrambler_seed,    sizeof(u32));
	c2 = compute_ip_checksum((u8 *)&pei_data->scrambler_seed_s3, sizeof(u32));
	checksum = add_ip_checksums(sizeof(u32), c1, c2);

	cmos_write((checksum >> 0) & 0xff, CMOS_OFFSET_MRC_SEED_CHK);
	cmos_write((checksum >> 8) & 0xff, CMOS_OFFSET_MRC_SEED_CHK + 1);
}

static void prepare_mrc_cache(struct pei_data *pei_data)
{
	u16 c1, c2, checksum, seed_checksum;
	size_t mrc_size;

	/* Preset just in case there is an error */
	pei_data->mrc_input = NULL;
	pei_data->mrc_input_len = 0;

	/* Read scrambler seeds from CMOS */
	pei_data->scrambler_seed = cmos_read32(CMOS_OFFSET_MRC_SEED);
	printk(BIOS_DEBUG, "Read scrambler seed    0x%08x from CMOS 0x%02x\n",
	       pei_data->scrambler_seed, CMOS_OFFSET_MRC_SEED);

	pei_data->scrambler_seed_s3 = cmos_read32(CMOS_OFFSET_MRC_SEED_S3);
	printk(BIOS_DEBUG, "Read S3 scrambler seed 0x%08x from CMOS 0x%02x\n",
	       pei_data->scrambler_seed_s3, CMOS_OFFSET_MRC_SEED_S3);

	/* Compute seed checksum and compare */
	c1 = compute_ip_checksum((u8 *)&pei_data->scrambler_seed,    sizeof(u32));
	c2 = compute_ip_checksum((u8 *)&pei_data->scrambler_seed_s3, sizeof(u32));
	checksum = add_ip_checksums(sizeof(u32), c1, c2);

	seed_checksum  = cmos_read(CMOS_OFFSET_MRC_SEED_CHK);
	seed_checksum |= cmos_read(CMOS_OFFSET_MRC_SEED_CHK + 1) << 8;

	if (checksum != seed_checksum) {
		printk(BIOS_ERR, "%s: invalid seed checksum\n", __func__);
		pei_data->scrambler_seed = 0;
		pei_data->scrambler_seed_s3 = 0;
		return;
	}

	pei_data->mrc_input = mrc_cache_current_mmap_leak(MRC_TRAINING_DATA,
							  MRC_CACHE_VERSION,
							  &mrc_size);
	if (pei_data->mrc_input == NULL) {
		/* Error message printed in find_current_mrc_cache */
		return;
	}

	pei_data->mrc_input_len = mrc_size;

	printk(BIOS_DEBUG, "%s: at %p, size %zx\n", __func__,
	       pei_data->mrc_input, mrc_size);
}

/**
 * Find PEI executable in coreboot filesystem and execute it.
 *
 * @param pei_data: configuration data for UEFI PEI reference code
 */
void sdram_initialize(struct pei_data *pei_data)
{
	int (*entry)(struct pei_data *pei_data) __attribute__((regparm(1)));

	/* Wait for ME to be ready */
	intel_early_me_init();
	intel_early_me_uma_size();

	printk(BIOS_DEBUG, "Starting UEFI PEI System Agent\n");

	/*
	 * Do not pass MRC data in for recovery mode boot,
	 * Always pass it in for S3 resume.
	 */
	if (!(CONFIG(SANDYBRIDGE_VBOOT_IN_BOOTBLOCK) && vboot_recovery_mode_enabled()) ||
	    pei_data->boot_mode == 2)
		prepare_mrc_cache(pei_data);

	/* If MRC data is not found we cannot continue S3 resume. */
	if (pei_data->boot_mode == 2 && !pei_data->mrc_input) {
		printk(BIOS_DEBUG, "Giving up in %s: No MRC data\n", __func__);
		system_reset();
	}

	/* Pass console handler in pei_data */
	pei_data->tx_byte = do_putchar;

	/* Locate and call UEFI System Agent binary. */
	entry = cbfs_boot_map_with_leak("mrc.bin", CBFS_TYPE_MRC, NULL);
	if (entry) {
		int rv;
		rv = entry (pei_data);
		if (rv) {
			switch (rv) {
			case -1:
				printk(BIOS_ERR, "PEI version mismatch.\n");
				break;
			case -2:
				printk(BIOS_ERR, "Invalid memory frequency.\n");
				break;
			default:
				printk(BIOS_ERR, "MRC returned %x.\n", rv);
			}
			die_with_post_code(POST_INVALID_VENDOR_BINARY,
					   "Nonzero MRC return value.\n");
		}
	} else {
		die("UEFI PEI System Agent not found.\n");
	}

	/* mrc.bin reconfigures USB, so reinit it to have debug */
	if (CONFIG(USBDEBUG_IN_PRE_RAM))
		usbdebug_hw_init(true);

	/* For reference, print the System Agent version after executing the UEFI PEI stage */
	u32 version = MCHBAR32(MRC_REVISION);
	printk(BIOS_DEBUG, "System Agent Version %d.%d.%d Build %d\n",
		(version >> 24) & 0xff, (version >> 16) & 0xff,
		(version >>  8) & 0xff, (version >>  0) & 0xff);

	/*
	 * Send ME init done for SandyBridge here.
	 * This is done inside the SystemAgent binary on IvyBridge.
	 */
	if (BASE_REV_SNB == (pci_read_config16(PCI_CPU_DEVICE, PCI_DEVICE_ID) & BASE_REV_MASK))
		intel_early_me_init_done(ME_INIT_STATUS_SUCCESS);
	else
		intel_early_me_status();

	report_memory_config();
}

/*
 * These are the location and structure of MRC_VAR data in CAR.
 * The CAR region looks like this:
 * +------------------+ -> DCACHE_RAM_BASE
 * |                  |
 * |                  |
 * |  COREBOOT STACK  |
 * |                  |
 * |                  |
 * +------------------+ -> DCACHE_RAM_BASE + DCACHE_RAM_SIZE
 * |                  |
 * |  MRC HEAP        |
 * |  size = 0x5000   |
 * |                  |
 * +------------------+
 * |                  |
 * |  MRC VAR         |
 * |  size = 0x4000   |
 * |                  |
 * +------------------+ -> DACHE_RAM_BASE + DACHE_RAM_SIZE
 *                               + DCACHE_RAM_MRC_VAR_SIZE
 */
#define DCACHE_RAM_MRC_VAR_BASE	 (CONFIG_DCACHE_RAM_BASE + CONFIG_DCACHE_RAM_SIZE \
				+ CONFIG_DCACHE_RAM_MRC_VAR_SIZE - 0x4000)

struct mrc_var_data {
	u32 acpi_timer_flag;
	u32 pool_used;
	u32 pool_base;
	u32 tx_byte;
	u32 reserved[4];
} __packed;

static void northbridge_fill_pei_data(struct pei_data *pei_data)
{
	pei_data->mchbar       = (uintptr_t)DEFAULT_MCHBAR;
	pei_data->dmibar       = (uintptr_t)DEFAULT_DMIBAR;
	pei_data->epbar        = DEFAULT_EPBAR;
	pei_data->pciexbar     = CONFIG_MMCONF_BASE_ADDRESS;
	pei_data->hpet_address = CONFIG_HPET_ADDRESS;
	pei_data->thermalbase  = 0xfed08000;
	pei_data->system_type  = !(get_platform_type() == PLATFORM_MOBILE);
	pei_data->tseg_size    = CONFIG_SMM_TSEG_SIZE;

	if ((cpu_get_cpuid() & 0xffff0) == 0x306a0) {
		const struct device *dev = pcidev_on_root(1, 0);
		pei_data->pcie_init = dev && dev->enabled;
	} else {
		pei_data->pcie_init = 0;
	}
}

static void southbridge_fill_pei_data(struct pei_data *pei_data)
{
	const struct device *dev = pcidev_on_root(0x19, 0);

	pei_data->smbusbar   = CONFIG_FIXED_SMBUS_IO_BASE;
	pei_data->wdbbar     = 0x04000000;
	pei_data->wdbsize    = 0x1000;
	pei_data->rcba       = (uintptr_t)DEFAULT_RCBA;
	pei_data->pmbase     = DEFAULT_PMBASE;
	pei_data->gpiobase   = DEFAULT_GPIOBASE;
	pei_data->gbe_enable = dev && dev->enabled;
}

static void devicetree_fill_pei_data(struct pei_data *pei_data)
{
	const struct northbridge_intel_sandybridge_config *cfg;

	const struct device *dev = pcidev_on_root(0, 0);
	if (!dev || !dev->chip_info)
		return;

	cfg = dev->chip_info;

	switch (cfg->max_mem_clock_mhz) {
	/* MRC only supports fixed numbers of frequencies */
	default:
		printk(BIOS_WARNING, "RAMINIT: Limiting DDR3 clock to 800 Mhz\n");
		/* fallthrough */
	case 400:
		pei_data->max_ddr3_freq = 800;
		break;
	case 533:
		pei_data->max_ddr3_freq = 1066;
		break;
	case 666:
		pei_data->max_ddr3_freq = 1333;
		break;
	case 800:
		pei_data->max_ddr3_freq = 1600;
		break;

	}

	memcpy(pei_data->spd_addresses, cfg->spd_addresses, sizeof(pei_data->spd_addresses));
	memcpy(pei_data->ts_addresses,  cfg->ts_addresses,  sizeof(pei_data->ts_addresses));

	pei_data->ec_present     = cfg->ec_present;
	pei_data->ddr3lv_support = cfg->ddr3lv_support;

	pei_data->nmode = cfg->nmode;
	pei_data->ddr_refresh_rate_config = cfg->ddr_refresh_rate_config;

	memcpy(pei_data->usb_port_config, cfg->usb_port_config,
	       sizeof(pei_data->usb_port_config));

	pei_data->usb3.mode                = cfg->usb3.mode;
	pei_data->usb3.hs_port_switch_mask = cfg->usb3.hs_port_switch_mask;
	pei_data->usb3.preboot_support     = cfg->usb3.preboot_support;
	pei_data->usb3.xhci_streams        = cfg->usb3.xhci_streams;
}

static void disable_p2p(void)
{
	/* Disable PCI-to-PCI bridge early to prevent probing by MRC */
	const struct device *const p2p = pcidev_on_root(0x1e, 0);
	if (p2p && p2p->enabled)
		return;

	RCBA32(FD) |= PCH_DISABLE_P2P;
}

void perform_raminit(int s3resume)
{
	struct pei_data pei_data;
	struct mrc_var_data *mrc_var;

	/* Prepare USB controller early in S3 resume */
	if (!mainboard_should_reset_usb(s3resume))
		enable_usb_bar();

	memset(&pei_data, 0, sizeof(pei_data));
	pei_data.pei_version = PEI_VERSION,

	northbridge_fill_pei_data(&pei_data);
	southbridge_fill_pei_data(&pei_data);
	devicetree_fill_pei_data(&pei_data);
	mainboard_fill_pei_data(&pei_data);

	post_code(0x3a);

	/* Fill after mainboard_fill_pei_data as it might provide spd_data */
	pei_data.dimm_channel0_disabled =
		(!pei_data.spd_addresses[0] && !pei_data.spd_data[0][0]) +
		(!pei_data.spd_addresses[1] && !pei_data.spd_data[1][0]) * 2;

	pei_data.dimm_channel1_disabled =
		(!pei_data.spd_addresses[2] && !pei_data.spd_data[2][0]) +
		(!pei_data.spd_addresses[3] && !pei_data.spd_data[3][0]) * 2;

	/* Fix spd_data. MRC only uses spd_data[0] and ignores the other */
	for (size_t i = 1; i < ARRAY_SIZE(pei_data.spd_data); i++) {
		if (pei_data.spd_data[i][0] && !pei_data.spd_data[0][0]) {
			memcpy(pei_data.spd_data[0], pei_data.spd_data[i],
			       sizeof(pei_data.spd_data[0]));

		} else if (pei_data.spd_data[i][0] && pei_data.spd_data[0][0]) {
			if (memcmp(pei_data.spd_data[i], pei_data.spd_data[0],
			    sizeof(pei_data.spd_data[0])) != 0)
				die("Onboard SPDs must match each other");
		}
	}

	disable_p2p();

	pei_data.boot_mode = s3resume ? 2 : 0;
	timestamp_add_now(TS_BEFORE_INITRAM);
	sdram_initialize(&pei_data);

	/* Sanity check mrc_var location by verifying a known field */
	mrc_var = (void *)DCACHE_RAM_MRC_VAR_BASE;
	if (mrc_var->tx_byte == (uintptr_t)pei_data.tx_byte) {
		printk(BIOS_DEBUG, "MRC_VAR pool occupied [%08x,%08x]\n",
		       mrc_var->pool_base, mrc_var->pool_base + mrc_var->pool_used);

	} else {
		printk(BIOS_ERR, "Could not parse MRC_VAR data\n");
		hexdump32(BIOS_ERR, mrc_var, sizeof(*mrc_var) / sizeof(u32));
	}

	const int cbmem_was_initted = !cbmem_recovery(s3resume);
	if (!s3resume)
		save_mrc_data(&pei_data);

	if (s3resume && !cbmem_was_initted) {
		/* Failed S3 resume, reset to come up cleanly */
		system_reset();
	}
}
