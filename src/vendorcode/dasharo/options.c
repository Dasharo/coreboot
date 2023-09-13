/* SPDX-License-Identifier: GPL-2.0-only */

#include <console/console.h>
#include <dasharo/options.h>
#include <drivers/efi/efivars.h>
#include <option.h>
#include <smmstore.h>
#include <types.h>

static const EFI_GUID dasharo_system_features_guid = {
	0xd15b327e, 0xff2d, 0x4fc1, { 0xab, 0xf6, 0xc1, 0x2b, 0xd0, 0x8c, 0x13, 0x59 }
};

/* Value of *var is not changed on failure, so it's safe to initialize it with
 * a default before the call. */
static enum cb_err read_u8_var(const char *var_name, uint8_t *var)
{
	struct region_device rdev;

	if (!smmstore_lookup_region(&rdev)) {
		uint8_t tmp;
		uint32_t size = sizeof(tmp);
		enum cb_err ret = efi_fv_get_option(&rdev, &dasharo_system_features_guid,
						    var_name, &tmp, &size);
		if (ret == CB_SUCCESS) {
			if (size == sizeof(tmp)) {
				*var = tmp;
				return CB_SUCCESS;
			}

			printk(BIOS_WARNING,
			       "dasharo: EFI variable \"%s\" of wrong size (%d instead of 1)\n",
			       var_name, size);
		}
	}

	return CB_ERR;
}

/* Value of *var is not changed on failure, so it's safe to initialize it with
 * a default before the call. */
static enum cb_err read_bool_var(const char *var_name, bool *var)
{
	uint8_t tmp;

	if (read_u8_var(var_name, &tmp) == CB_SUCCESS) {
		*var = tmp != 0;
		return CB_SUCCESS;
	}

	return CB_ERR;
}

uint8_t dasharo_get_power_on_after_fail(void)
{
	uint8_t power_status;

	power_status = get_uint_option("power_on_after_fail",
				       CONFIG_MAINBOARD_POWER_FAILURE_STATE);

	/* When present, EFI variable has higher priority. */
	if (CONFIG(DRIVERS_EFI_VARIABLE_STORE))
		read_u8_var("PowerFailureState", &power_status);

	return power_status;
}

bool dasharo_resizeable_bars_enabled(void)
{
	bool enabled = false;

	if (CONFIG(DRIVERS_EFI_VARIABLE_STORE) && CONFIG(PCIEXP_SUPPORT_RESIZABLE_BARS))
		read_bool_var("PCIeResizeableBarsEnabled", &enabled);

	return enabled;
}


bool is_vboot_locking_permitted(void)
{
	bool lock = true;
	bool fum = false;

	if (CONFIG(DRIVERS_EFI_VARIABLE_STORE))
		read_bool_var("FirmwareUpdateMode", &fum);

	/* Disable lock if in Firmware Update Mode */
	if (fum)
		return false;

	if (CONFIG(DRIVERS_EFI_VARIABLE_STORE))
		read_bool_var("LockBios", &lock);

	return lock;
}

