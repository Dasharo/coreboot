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

bool dma_protection_enabled(void)
{
	struct region_device rdev;
	enum cb_err ret = CB_EFI_OPTION_NOT_FOUND;
	struct iommu_config {
		bool iommu_enable;
		bool iommu_handoff;
	} __packed iommu_var;
	uint32_t size;

	if (smmstore_lookup_region(&rdev))
		return false;

	size = sizeof(iommu_var);
	if (CONFIG(DRIVERS_EFI_VARIABLE_STORE))
		ret = efi_fv_get_option(&rdev, &dasharo_system_features_guid, "IommuConfig",
				&iommu_var, &size);

	if (ret != CB_SUCCESS)
		return false;

	return iommu_var.iommu_enable;
}

uint8_t cse_get_me_disable_mode(void)
{
	uint8_t var = ME_MODE_ENABLE;
	bool fum = false;

	if (CONFIG(DRIVERS_EFI_VARIABLE_STORE))
		read_bool_var("FirmwareUpdateMode", &fum);

	/* Disable ME if in Firmware Update Mode */
	if (fum)
		return ME_MODE_DISABLE_HAP;

	if (CONFIG(DRIVERS_EFI_VARIABLE_STORE))
		read_u8_var("MeMode", &var);

	return var;
}

bool is_smm_bwp_permitted(void)
{
	bool smm_bwp = false;
	bool fum = false;

	if (CONFIG(DRIVERS_EFI_VARIABLE_STORE))
		read_bool_var("FirmwareUpdateMode", &fum);

	/* Disable SMM BWP if in Firmware Update Mode */
	if (fum)
		return false;

	if (CONFIG(DRIVERS_EFI_VARIABLE_STORE))
		read_bool_var("SmmBwp", &smm_bwp);

	return smm_bwp;
}

void get_watchdog_config(struct watchdog_config *wdt_cfg)
{
	struct region_device rdev;
	enum cb_err ret = CB_EFI_OPTION_NOT_FOUND;
	uint32_t size;

	wdt_cfg->wdt_enable = false;
	wdt_cfg->wdt_timeout = CONFIG_SOC_INTEL_COMMON_OC_WDT_TIMEOUT_SECONDS;

	if (smmstore_lookup_region(&rdev))
		return;

	size = sizeof(*wdt_cfg);
	if (CONFIG(DRIVERS_EFI_VARIABLE_STORE))
		ret = efi_fv_get_option(&rdev, &dasharo_system_features_guid, "WatchdogConfig",
					wdt_cfg, &size);

	if (ret != CB_SUCCESS || size != sizeof(*wdt_cfg)) {
		wdt_cfg->wdt_enable = false;
		wdt_cfg->wdt_timeout = CONFIG_SOC_INTEL_COMMON_OC_WDT_TIMEOUT_SECONDS;
	}
}

bool get_ps2_option(void)
{
	bool ps2_en = true;

	if (CONFIG(DRIVERS_EFI_VARIABLE_STORE))
		read_bool_var("Ps2Controller", &ps2_en);

	return ps2_en;
}

uint8_t get_sleep_type_option(void)
{
	uint8_t sleep_type = SLEEP_TYPE_OPTION_S0IX;

	if (CONFIG(DRIVERS_EFI_VARIABLE_STORE))
		read_u8_var("SleepType", &sleep_type);

	return sleep_type;
}


uint8_t get_fan_curve_option(void)
{
	uint8_t fan_curve = FAN_CURVE_OPTION_SILENT;

	if (CONFIG(DRIVERS_EFI_VARIABLE_STORE))
		read_u8_var("FanCurveOption", &fan_curve);

	return fan_curve;
}

bool get_camera_option(void)
{
	bool cam_en = CAMERA_ENABLED_OPTION;

	if (CONFIG(DRIVERS_EFI_VARIABLE_STORE))
		read_bool_var("EnableCamera", &cam_en);

	return cam_en;
}
