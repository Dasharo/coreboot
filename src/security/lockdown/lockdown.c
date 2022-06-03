/* SPDX-License-Identifier: GPL-2.0-or-later */

#include <boot_device.h>
#include <commonlib/region.h>
#include <console/console.h>
#include <drivers/efi/efivars.h>
#include <bootstate.h>
#include <fmap.h>
#include <smmstore.h>

#include <Uefi/UefiBaseType.h>

static const EFI_GUID dasharo_system_features_guid = {
	0xd15b327e, 0xff2d, 0x4fc1, { 0xab, 0xf6, 0xc1, 0x2b, 0xd0, 0x8c, 0x13, 0x59 }
};

/*
 * Checks whether protecting vboot partition was enabled in BIOS.
 */
static bool is_vboot_locking_permitted(void)
{
#if CONFIG(DRIVERS_EFI_VARIABLE_STORE)
	struct region_device rdev;
	enum cb_err ret;
	uint8_t var;
	uint32_t size;

	if (smmstore_lookup_region(&rdev))
		return false;

	size = sizeof(var);
	ret = efi_fv_get_option(&rdev, &dasharo_system_features_guid, "LockBios", &var, &size);
	if (ret != CB_SUCCESS)
		return true;

	return var;
#else
	/* In absence of EFI variables, follow compilation options */
	return true;
#endif
}

/*
 * Enables read- /write protection of the bootmedia.
 */
void boot_device_security_lockdown(void)
{
	const struct region_device *rdev = NULL;
	struct region_device dev;
	enum bootdev_prot_type lock_type;

	if (!is_vboot_locking_permitted()) {
		printk(BIOS_DEBUG, "BM-LOCKDOWN: Skipping enabling boot media protection\n");
		return;
	}

	printk(BIOS_DEBUG, "BM-LOCKDOWN: Enabling boot media protection scheme ");

	if (CONFIG(BOOTMEDIA_LOCK_CONTROLLER)) {
		if (CONFIG(BOOTMEDIA_LOCK_WHOLE_RO)) {
			printk(BIOS_DEBUG, "'readonly'");
			lock_type = CTRLR_WP;
		} else if (CONFIG(BOOTMEDIA_LOCK_WHOLE_NO_ACCESS)) {
			printk(BIOS_DEBUG, "'no access'");
			lock_type = CTRLR_RWP;
		} else if (CONFIG(BOOTMEDIA_LOCK_WPRO_VBOOT_RO)) {
			printk(BIOS_DEBUG, "'WP_RO only'");
			lock_type = CTRLR_WP;
		}
		printk(BIOS_DEBUG, " using CTRL...\n");
	} else {
		if (CONFIG(BOOTMEDIA_LOCK_WHOLE_RO)) {
			printk(BIOS_DEBUG, "'readonly'");
			lock_type = MEDIA_WP;
		} else if (CONFIG(BOOTMEDIA_LOCK_WPRO_VBOOT_RO)) {
			printk(BIOS_DEBUG, "'WP_RO only'");
			lock_type = MEDIA_WP;
		}
		printk(BIOS_DEBUG, " using flash chip...\n");
	}

	if (CONFIG(BOOTMEDIA_LOCK_WPRO_VBOOT_RO)) {
		if (fmap_locate_area_as_rdev("WP_RO", &dev) < 0)
			printk(BIOS_ERR, "BM-LOCKDOWN: Could not find region 'WP_RO'\n");
		else
			rdev = &dev;
	} else {
		rdev = boot_device_ro();
	}

	if (rdev && boot_device_wp_region(rdev, lock_type) >= 0)
		printk(BIOS_INFO, "BM-LOCKDOWN: Enabled bootmedia protection\n");
	else
		printk(BIOS_ERR, "BM-LOCKDOWN: Failed to enable bootmedia protection\n");
}

static void lock(void *unused)
{
	boot_device_security_lockdown();
}

/*
 * Keep in sync with mrc_cache.c
 */

#if CONFIG(MRC_WRITE_NV_LATE)
BOOT_STATE_INIT_ENTRY(BS_OS_RESUME_CHECK, BS_ON_EXIT, lock, NULL);
#else
BOOT_STATE_INIT_ENTRY(BS_DEV_RESOURCES, BS_ON_ENTRY, lock, NULL);
#endif
