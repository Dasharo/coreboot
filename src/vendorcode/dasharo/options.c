/* SPDX-License-Identifier: GPL-2.0-only */

#include <bootstate.h>
#include <console/console.h>
#include <dasharo/options.h>
#include <device/mmio.h>
#include <drivers/efi/efivars.h>
#include <fmap.h>
#include <option.h>
#include <pc80/mc146818rtc.h>
#include <soc/intel/common/reset.h>
#include <smmstore.h>
#include <types.h>
#include <uuid.h>

#if CONFIG(SOC_INTEL_COMMON_BLOCK_CSE)
#include <intelblocks/cse.h>
#endif

_Static_assert(sizeof(bool) == 1, "Size of bool, must be 1");

/* Keeping field names as in EDK to make things easier to follow, types are
 * different though. */
struct apu_config_t {
	bool CorePerfBoost;
	bool WatchdogEnable;
	uint16_t WatchdogTimeout; /* in seconds */
	bool PciePwrMgmt;
} __packed;

static const EFI_GUID dasharo_system_features_guid = {
	0xd15b327e, 0xff2d, 0x4fc1, { 0xab, 0xf6, 0xc1, 0x2b, 0xd0, 0x8c, 0x13, 0x59 }
};

static const EFI_GUID dasharo_apu_features_guid = {
	0x6f4e051b, 0x1c10, 0x422a, { 0x98, 0xcf, 0x96, 0x2e, 0x78, 0x36, 0x5c, 0x74 }
};

/* Value of *var is not changed on failure, so it's safe to initialize it with
 * a default before the call. */
static enum cb_err read_u8_var(const char *var_name, uint8_t *var)
{
	struct region_device rdev;

	/* Report failure so that callers fall back to their default. Returning
	 * CB_SUCCESS without touching *var would make read_bool_var() below
	 * evaluate an uninitialized value. */
	if (cmos_is_invalid())
		return CB_ERR;

	if (CONFIG(SMMSTORE_V2) && !smmstore_lookup_region(&rdev)) {
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

/* Never fails, just returns defaults on error. */
static struct apu_config_t get_apu_config(void)
{
	static bool init_done;
	static struct apu_config_t cfg;

	if (init_done)
		return cfg;

	/* Start with defaults as in EDK. */
	struct apu_config_t val = {
		.CorePerfBoost = true,
		.WatchdogEnable = false,
		.WatchdogTimeout = 60,
		.PciePwrMgmt = false,
	};

	cfg = val;

	if (cmos_is_invalid()) {
		init_done = true;
		return cfg;
	}

	struct region_device rdev;
	if (CONFIG(SMMSTORE_V2) && !smmstore_lookup_region(&rdev)) {
		uint32_t size = sizeof(val);
		enum cb_err ret = efi_fv_get_option(&rdev,
						    &dasharo_apu_features_guid,
						    "ApuConfig", &val, &size);
		/* Deliberately allowing mismatched size of structure to allow
		 * compatible changes. All known fields are initialized and
		 * extra fields are irrelevant. */
		if (ret == CB_SUCCESS)
			cfg = val;
	}

	init_done = true;
	return cfg;
}

enum cb_err dasharo_reset_options(void)
{
	struct region_device rdev;
	ssize_t res;

	if (!CONFIG(SMMSTORE_V2) || smmstore_lookup_region(&rdev))
		return CB_ERR;

	res = rdev_eraseat(&rdev, 0, region_device_sz(&rdev));
	if (res != region_device_sz(&rdev))
		return CB_ERR;

	return CB_SUCCESS;
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

uint8_t dasharo_get_usb_port_power(void)
{
	uint8_t usb_port_power = USB_PORT_ON_WHEN_POWERED;

	/* When present, EFI variable has higher priority. */
	if (CONFIG(DRIVERS_EFI_VARIABLE_STORE))
		read_u8_var("UsbPortPower", &usb_port_power);

	if (usb_port_power > USB_PORT_ALWAYS_ON)
		return USB_PORT_ON_WHEN_POWERED;

	return usb_port_power;
}

bool dasharo_resizeable_bars_enabled(void)
{
	static bool enabled = false;
	static bool read_once = false;

	if (!CONFIG(PCIEXP_SUPPORT_RESIZABLE_BARS))
		return false;

	if (CONFIG(DRIVERS_EFI_VARIABLE_STORE)) {
		if (!read_once)
			read_bool_var("PCIeResizeableBarsEnabled", &enabled);
		/*
		 * This variable would be read for each PCIe device.
		 * Avoid it by reading the variable only once.
		 */
		read_once = true;
	} else {
		/* If no variables are not enabled, follow Kconfig */
		return true;
	}

	return enabled;
}

static bool fum_is_active(void)
{
	bool fum = false;

	if (!CONFIG(DASHARO_FIRMWARE_UPDATE_MODE))
		return false;

	/* Handling capsules implies active FUM. */
	if (dasharo_is_disk_capsules_boot())
		return true;

	/* Check the FUM Request variable. FUM Active var is created by the payload. */
	if (CONFIG(DRIVERS_EFI_VARIABLE_STORE))
		read_bool_var("FirmwareUpdateModeRequest", &fum);

	return fum;
}

bool is_vboot_locking_permitted(void)
{
	bool lock = true;

	if (!CONFIG(DRIVERS_EFI_VARIABLE_STORE))
		return !CONFIG(BOOTMEDIA_LOCK_NONE);

	/* Disable lock if in Firmware Update Mode */
	if (fum_is_active())
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

	if (!CONFIG(SMMSTORE_V2) || smmstore_lookup_region(&rdev))
		return false;

	size = sizeof(iommu_var);
	if (CONFIG(DRIVERS_EFI_VARIABLE_STORE)) {
		if (cmos_is_invalid())
			return false;

		ret = efi_fv_get_option(&rdev, &dasharo_system_features_guid, "IommuConfig",
				&iommu_var, &size);
	}

	if (ret != CB_SUCCESS)
		return false;

	return iommu_var.iommu_enable;
}

#if CONFIG(SOC_INTEL_COMMON_BLOCK_CSE)
static void disable_me_hmrfpo(uint8_t *var)
{
	int hmrfpo_sts = cse_hmrfpo_get_status();

	if (hmrfpo_sts == -1) {
		printk (BIOS_DEBUG, "ME HMRFPO Get Status failed\n");
		goto hmrfpo_error;
	}

	printk (BIOS_DEBUG, "ME HMRFPO status: ");
	switch (hmrfpo_sts) {
	case 0:
		printk (BIOS_DEBUG, "DISABLED\n");
		if (cse_hmrfpo_enable() != CB_SUCCESS)
			goto hmrfpo_error;
		else
			do_global_reset(); /* No return */
		break;
	case 1:
		printk (BIOS_DEBUG, "LOCKED\n");
		goto hmrfpo_error;
	case 2:
		printk (BIOS_DEBUG, "ENABLED\n");
		/* Override ME disable method to not switch ME mode */
		*var =  ME_MODE_DISABLE_HMRFPO;
		/* TODO: disable HMRFPO here if not in FUM? Reboot disables HMRFPO. */
		break;
	default:
		printk (BIOS_DEBUG, "UNKNOWN\n");
		goto hmrfpo_error;
	}

	return;

hmrfpo_error:
	/* Try other available methods if something goes wrong */
	*var = CONFIG(HAVE_INTEL_ME_HAP) ? ME_MODE_DISABLE_HAP
					 : ME_MODE_DISABLE_HECI;
}

uint8_t cse_get_me_disable_mode(void)
{
	uint8_t var = CONFIG_INTEL_ME_DEFAULT_STATE;

	if (CONFIG(DRIVERS_EFI_VARIABLE_STORE))
		read_u8_var("MeMode", &var);

	/*
	 * Disable ME via HMRPFO if in Firmware Update Mode
	 *
	 * If capsules are supported, don't do this unless it's on-disk
	 * capsules, because in-RAM capsules do not survive a cold reset
	 * required for HMRFPO.
	 */
	if (fum_is_active() &&
	    (!CONFIG(DRIVERS_EFI_UPDATE_CAPSULES) || dasharo_is_disk_capsules_boot())) {
		/* Check if already in HMRFPO mode */
		if (cse_is_hfs1_com_secover_mei_msg())
			return ME_MODE_DISABLE_HMRFPO;
		else
			disable_me_hmrfpo(&var);
	}

	return var;
}

enum dgpu_state dasharo_dgpu_state(void)
{
	uint8_t dgpu_state = NVIDIA_OPTIMUS;
	/*
	 * 0 - IGPU_ONLY
	 * 1 - NVIDIA_OPTIMUS (iGPU+dGPU)
	 * 2 - DGPU_ONLY
	 */
	if (CONFIG(DRIVERS_EFI_VARIABLE_STORE))
		read_u8_var("DGPUState", &dgpu_state);

	return dgpu_state;
}

#else
uint8_t cse_get_me_disable_mode(void)
{
	return 0;
}
#endif

bool is_smm_bwp_permitted(void)
{
	bool smm_bwp = false;

	/* Disable SMM BWP if in Firmware Update Mode */
	if (fum_is_active())
		return false;

	if (CONFIG(DRIVERS_EFI_VARIABLE_STORE))
		read_bool_var("SmmBwp", &smm_bwp);
	else
		return CONFIG(BOOTMEDIA_SMM_BWP);

	return smm_bwp;
}

void get_watchdog_config(struct watchdog_config *wdt_cfg)
{
	struct region_device rdev;
	enum cb_err ret = CB_EFI_OPTION_NOT_FOUND;
	uint32_t size;

	wdt_cfg->wdt_enable = CONFIG(SOC_INTEL_COMMON_OC_WDT_ENABLE);
	wdt_cfg->wdt_timeout = CONFIG_SOC_INTEL_COMMON_OC_WDT_TIMEOUT_SECONDS;

	if (!CONFIG(SMMSTORE_V2) || smmstore_lookup_region(&rdev))
		return;

	size = sizeof(*wdt_cfg);
	if (CONFIG(DRIVERS_EFI_VARIABLE_STORE)) {
		if (cmos_is_invalid())
			return;

		ret = efi_fv_get_option(&rdev, &dasharo_system_features_guid, "WatchdogConfig",
					wdt_cfg, &size);
	}

	if (ret != CB_SUCCESS || size != sizeof(*wdt_cfg)) {
		wdt_cfg->wdt_enable = CONFIG(SOC_INTEL_COMMON_OC_WDT_ENABLE);
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
	uint8_t sleep_type = CONFIG(DASHARO_PREFER_S3_SLEEP) ? SLEEP_TYPE_OPTION_S3
							     : SLEEP_TYPE_OPTION_S0IX;

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

bool get_wireless_option(void)
{
	bool wireless_en = true;

	if (CONFIG(DRIVERS_EFI_VARIABLE_STORE))
		read_bool_var("EnableWifiBt", &wireless_en);

	return wireless_en;
}

void get_battery_config(struct battery_config *bat_cfg)
{
	struct region_device rdev;
	enum cb_err ret = CB_EFI_OPTION_NOT_FOUND;
	uint32_t size;

	bat_cfg->start_threshold = BATTERY_START_THRESHOLD_DEFAULT;
	bat_cfg->stop_threshold = BATTERY_STOP_THRESHOLD_DEFAULT;

	if (!CONFIG(SMMSTORE_V2) || smmstore_lookup_region(&rdev))
		return;

	size = sizeof(*bat_cfg);
	if (CONFIG(DRIVERS_EFI_VARIABLE_STORE)) {
		if (cmos_is_invalid())
			return;

		ret = efi_fv_get_option(&rdev, &dasharo_system_features_guid, "BatteryConfig",
					bat_cfg, &size);
	}

	if (ret != CB_SUCCESS || size != sizeof(*bat_cfg)) {
		bat_cfg->start_threshold = BATTERY_START_THRESHOLD_DEFAULT;
		bat_cfg->stop_threshold = BATTERY_STOP_THRESHOLD_DEFAULT;
	}
}

#define MAX_SERIAL_LENGTH 0x100

bool get_serial_number_from_efivar(char *serial_number)
{
	struct region_device rdev;
	enum cb_err ret = CB_EFI_OPTION_NOT_FOUND;
	uint32_t size;

	if (!CONFIG(SMMSTORE_V2) || smmstore_lookup_region(&rdev))
		return false;

	size = MAX_SERIAL_LENGTH;
	if (CONFIG(DRIVERS_EFI_VARIABLE_STORE))
		ret = efi_fv_get_option(&rdev, &dasharo_system_features_guid, "Type2SN", serial_number, &size);

	if (ret != CB_SUCCESS || size > MAX_SERIAL_LENGTH)
		return false;

	serial_number[size] = '\0';

	return true;
}


bool get_uuid_from_efivar(uint8_t *uuid)
{
	struct region_device rdev;
	enum cb_err ret = CB_EFI_OPTION_NOT_FOUND;
	uint32_t size;

	if (!CONFIG(SMMSTORE_V2) || smmstore_lookup_region(&rdev))
		return false;

	size = UUID_LEN;
	ret = efi_fv_get_option(&rdev, &dasharo_system_features_guid, "Type1UUID", uuid, &size);
	if (ret != CB_SUCCESS || size != UUID_LEN)
		return false;

	return true;
}

uint8_t dasharo_get_memory_profile(void)
{
	/* Using default SPD/JEDEC profile by default. */
	uint8_t profile = 0;

	if (CONFIG(DRIVERS_EFI_VARIABLE_STORE))
		read_u8_var("MemoryProfile", &profile);

	return profile;
}

bool dasharo_apu_pcie_pm_enabled(void)
{
	return get_apu_config().PciePwrMgmt;
}

uint16_t dasharo_apu_watchdog_timeout(void)
{
	struct apu_config_t cfg = get_apu_config();
	return cfg.WatchdogEnable ? cfg.WatchdogTimeout : 0;
}

bool dasharo_apu_cpu_boost_enabled(void)
{
	return get_apu_config().CorePerfBoost;
}


uint8_t get_active_core_count_option(void)
{
	uint8_t count = ALL_CORES_ACTIVE;

	if (CONFIG(DRIVERS_EFI_VARIABLE_STORE))
		read_u8_var("CoreActiveCount", &count);

	return count;
}

uint8_t get_active_small_core_count_option(void)
{
	uint8_t count = ALL_CORES_ACTIVE;

	if (CONFIG(DRIVERS_EFI_VARIABLE_STORE))
		read_u8_var("SmallCoreActiveCount", &count);

	return count;
}

bool get_hyper_threading_option(void)
{
	bool ht_en;

	ht_en = get_uint_option("hyper_threading", CONFIG(FSP_HYPERTHREADING));

	if (CONFIG(DRIVERS_EFI_VARIABLE_STORE))
		read_bool_var("HyperThreading", &ht_en);

	return ht_en;
}

uint8_t get_cpu_throttling_offset(uint8_t tcc_offset)
{
	uint8_t offset = tcc_offset;

	if (CONFIG(DRIVERS_EFI_VARIABLE_STORE))
		read_u8_var("CpuThrottlingOffset", &offset);

	return offset;
}

bool get_ibecc_option(bool ibecc_default)
{
	bool ibecc_en = ibecc_default;

	if (CONFIG(DRIVERS_EFI_VARIABLE_STORE))
		read_bool_var("IBECC", &ibecc_en);

	return ibecc_en;
}

bool dasharo_is_disk_capsules_boot(void)
{
	if (!CONFIG(EDK2_CAPSULE_ON_DISK_SUPPORT))
		return false;

	bool disk_capsules_boot = false;
	if (CONFIG(DRIVERS_EFI_VARIABLE_STORE))
		read_bool_var("DiskCapsulesBoot", &disk_capsules_boot);

	return disk_capsules_boot;
}

/* Flash Master 1 : HOST/BIOS */
#define FLMSTR1			0x80

/* Flash signature Offset */
#define FLASH_SIGN_OFFSET	0x10
#define FLMSTR_WR_SHIFT_V2	20
#define FLASH_VAL_SIGN		0xFF0A55A

#define SI_DESC_REGION		"SI_DESC"
/* From MTL it is larger, but we still just need the first 4K */
#define SI_DESC_SIZE		0x1000

/* It checks whether host (Flash Master 1) has write access to the Descriptor Region or not */
static bool is_descriptor_writeable(uint8_t *desc)
{
	/* Check flash has valid signature */
	if (read32((void *)(desc + FLASH_SIGN_OFFSET)) != FLASH_VAL_SIGN) {
		printk(BIOS_ERR, "Flash Descriptor is not valid\n");
		return 0;
	}
	/* Check host has write access to the Descriptor Region */
	if (!((read32((void *)(desc + FLMSTR1)) >> FLMSTR_WR_SHIFT_V2) & BIT(0)))
		return 0;

	return 1;
}

/*
 * This function sets an EFI variable to signal to EDK2 whether the descriptor
 * is locked, which affects the visibility and functionality of certain features
 */
static void set_descriptor_lockdown_option(void *unused)
{
	uint8_t si_desc_buf[SI_DESC_SIZE];
	struct region_device desc_rdev, smmstore_rdev;
	uint8_t descriptor_writeable;

	if (!CONFIG(INTEL_DESCRIPTOR_MODE_CAPABLE) || !CONFIG(SMMSTORE))
		return;

	if (smmstore_lookup_region(&smmstore_rdev))
		return;

	if (fmap_locate_area_as_rdev_rw(SI_DESC_REGION, &desc_rdev) < 0) {
		printk(BIOS_ERR, "Failed to locate %s in the FMAP\n", SI_DESC_REGION);
		return;
	}

	if (rdev_readat(&desc_rdev, si_desc_buf, 0, SI_DESC_SIZE) != SI_DESC_SIZE) {
		printk(BIOS_ERR, "Failed to read Descriptor Region from SPI Flash\n");
		return;
	}

	descriptor_writeable = is_descriptor_writeable(si_desc_buf);

	printk(BIOS_DEBUG, "Descriptor is %swriteable\n", descriptor_writeable ? "" : "not ");

	efi_fv_set_option(&smmstore_rdev,
			  &dasharo_system_features_guid,
			  "DescriptorWriteable",
			  &descriptor_writeable,
			  sizeof(descriptor_writeable));
}
BOOT_STATE_INIT_ENTRY(BS_POST_DEVICE, BS_ON_ENTRY, set_descriptor_lockdown_option, NULL);
