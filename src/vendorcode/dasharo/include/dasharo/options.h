/* SPDX-License-Identifier: GPL-2.0-only */

#ifndef DASHARO_OPTIONS_H
#define DASHARO_OPTIONS_H

#include <commonlib/bsd/compiler.h>
#include <types.h>

#define SLEEP_TYPE_OPTION_S0IX	0
#define SLEEP_TYPE_OPTION_S3	1

#define FAN_CURVE_OPTION_SILENT 0
#define FAN_CURVE_OPTION_PERFORMANCE 1

#define FAN_CURVE_OPTION_DEFAULT FAN_CURVE_OPTION_SILENT

#define SLEEP_TYPE_OPTION_S0IX	0
#define SLEEP_TYPE_OPTION_S3	1

#define CAMERA_ENABLED_OPTION 1
#define CAMERA_DISABLED_OPTION 0

#define CAMERA_ENABLEMENT_DEFAULT CAMERA_ENABLED_OPTION

#define BATTERY_START_THRESHOLD_DEFAULT 95
#define BATTERY_STOP_THRESHOLD_DEFAULT 98

enum cse_disable_mode {
	ME_MODE_ENABLE = 0,
	ME_MODE_DISABLE_HECI = 1,
	ME_MODE_DISABLE_HAP = 2,
	/* Not a variable value, but serves a purpose of disabling ME in boot flow */
	ME_MODE_DISABLE_HMRFPO = 10
};

struct watchdog_config {
	bool wdt_enable;
	uint16_t wdt_timeout;
} __packed;

struct fan_point {
	uint8_t temp;
	uint8_t duty;
} __packed;

struct fan_curve {
	struct fan_point point1;
	struct fan_point point2;
	struct fan_point point3;
	struct fan_point point4;
} __packed;

struct battery_config {
	uint8_t start_threshold;
	uint8_t stop_threshold;
} __packed;

/* Attempts to wipe the SMMSTORE region in order to reset EFI options.
 * UEFIPayload will populate it with default values upon boot.
 *
 * Result:
 *  - CB_SUCCESS - Success
 *  - CB_ERR - Failure, including partial wipe
 */
enum cb_err dasharo_reset_options(void);

/* Looks up "power_on_after_fail" option and Dasharo/"PowerFailureState"
 * variable.
 *
 * Result (same as MAINBOARD_POWER_STATE_*):
 *  - 0 - stay powered OFF
 *  - 1 - power ON
 *  - 2 - restore previous state
 */
uint8_t dasharo_get_power_on_after_fail(void);

/* Looks up Dasharo/"PCIeResizeableBarsEnabled" variable if resizable PCIe BARs
 * support was enabled at compile-time.
 *
 * Result:
 *  - true  - resizeable PCIe BARs are supported and enabled
 *  - false - resizeable PCIe BARs are disabled or not supported
 */
bool dasharo_resizeable_bars_enabled(void);

/* Looks Dasharo/"LockBios" variable if bootmedia lock was enabled at compile
 * time.
 *
 * Result:
 *  - true  - bootmedia lock is supported and enabled
 *  - false - bootmedia lock is disabled or not supported
 */
bool is_vboot_locking_permitted(void);

/* Looks Dasharo/"IommuConfig" variable if early DMA protection lock was
 * enabled at compile time.
 *
 * Result:
 *  - true  - DMA protection is supported and enabled
 *  - false - DMA protection is disabled or not supported
 */
bool dma_protection_enabled(void);

/* Looks Dasharo/"MeMode" variable if ME should be enabeld or disabled.
 *
 * Result:
 *  - 0 - ME enabled
 *  - 1 - ME soft disabled
 *  - 2 - ME HAP disabled
 */
uint8_t cse_get_me_disable_mode(void);

/* Looks Dasharo/"SmmBwp" variable if SMM BIOS Write Protection was
 * enabled at compile time.
 *
 * Result:
 *  - true  - DMA protection is supported and enabled
 *  - false - DMA protection is disabled or not supported
 */
bool is_smm_bwp_permitted(void);

/* Looks Dasharo/"WatchdogCOnfig" variable if OC Watchdog was
 * enabled at compile time.
 *
 * Result:
 *  - true  - DMA protection is supported and enabled
 *  - false - DMA protection is disabled or not supported
 */
void get_watchdog_config(struct watchdog_config *wdt_cfg);

/* Looks Dasharo/"WatchdogCOnfig" variable if PS/2 controller should be
 * enabled. The function should be called from mainboard code to disable
 * appropriate PNP device in devicetree.
 *
 * Result:
 *  - true  - PS/2 controller enabled
 *  - false - PS/2 controller disabled
 */
bool get_ps2_option(void);

/* Looks Dasharo/"SleepType" variable to check which sleep type should be
 * enabled.
 *
 * Result:
 *  - 0 - S0ix
 *  - 1 - S3
 */
uint8_t get_sleep_type_option(void);

/* Looks Dasharo/"FanCurveOption" variable to check which fan curve EC should
 * apply.
 *
 *
 * Result:
 *  - 0 - Silent profile
 *  - 1 - Performance profile
 */
uint8_t get_fan_curve_option(void);

/* Looks Dasharo/"EnableCamera" variable to checkif camera USB port should be
 * emabled.
 *
 * Result:
 *  - true - camera enabled
 *  - false - camera disabled
 */
bool get_camera_option(void);

/* Looks Dasharo/"EnableWifiBt" variable to check if WiFi and Bluetooth should
 * be enabled.
 *
 * Result:
 *  - true - Wireless radios enabled
 *  - false - Wireless radios disabled
 */
bool get_wireless_option(void);

/* Looks Dasharo/"BatteryConfig" variable to check battery configuration
 * (charge thresholds)
 *
 * Arguments:
 *  - bat_cfg - battery configuration parameters
 */
void get_battery_config(struct battery_config *bat_cfg);

/* Looks Dasharo/"Type2SN" variable to check if Serial Number was saved.
 *
 * Arguments:
 *  - serial_number - pointer where the serial number should be returned
 * Result:
 *  - true - Serial Number present in EFI variables
 *  - false -  Serial Number not present in EFI variables
 */
bool get_serial_number_from_efivar(char *serial_number);

/* Looks Dasharo/"Type1UUID" variable to check if system UUID was saved.
 *
 * Arguments:
 *  - uuid - pointer where the UUID should be returned
 * Result:
 *  - true - System UUID present in EFI variables
 *  - false -  SystemUUID not present in EFI variables
 */
bool get_uuid_from_efivar(uint8_t *uuid);

/* Looks up Dasharo/"MemoryProfile" variable to know which memory profile to
 * enable.
 *
 * Result (values of FSP_M_CONFIG::SpdProfileSelected):
 *  - 0 - Default SPD Profile
 *  - 1 - Custom Profile
 *  - 2 - XMP Profile 1
 *  - 3 - XMP Profile 2
 *  - 4 - XMP Profile 3
 *  - 5 - XMP User Profile 4
 *  - 6 - XMP User Profile 5
 */
uint8_t dasharo_get_memory_profile(void);

/* Looks up PciePwrMgmt field in APU/"ApuConfig" to know whether to enable
 * Clock Power Management, ASPM L0s and L1 features on PCI Express ports.
 *
 * Result:
 *  - true  - PM features on PCI express ports are enabled
 *  - false - PM features on PCI express ports are disabled
 */
bool dasharo_apu_pcie_pm_enabled(void);

/* Looks up WatchdogEnable and WatchdogTimeout fields in APU/"ApuConfig" to
 * know whether to enable watchdog on boot and what period to use.
 *
 * Result:
 *  - = 0 - watchdog is disabled
 *  - > 0 - watchdog is enabled and should be configured to reboot after
 *          returned number of seconds has passed
 */
uint16_t dasharo_apu_watchdog_timeout(void);

#endif /* DASHARO_OPTIONS_H */
