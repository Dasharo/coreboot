/* SPDX-License-Identifier: GPL-2.0-only */

/*
 * Include this file into a mainboard DSDT inside the PCI domain 0 scope and it
 * will expose the temperature sensor of the processor as a thermal zone.
 *
 * All Zen processor families are supported.
 *
 * If, for example, the PCI domain 0 is PCI0, include the following:
 *
 * Scope (\_SB.PCI0) {
 *     #include <soc/amd/common/acpi/thermal_zone_smu.asl>
 * }
 *
 */

#define SMU_THERMAL_BLK_OFFSET	0x59800

#define ACPI_TEMP_KELVIN_OFFSET	2732

#ifndef CPU_TEMP_CRIT_VALUE
# define CPU_TEMP_CRIT_VALUE	100
#endif

#ifndef CPU_TEMP_HOT_OFFSET
# define CPU_TEMP_HOT_OFFSET	50
#endif

#ifndef CPU_TEMP_PSV_OFFSET
# define CPU_TEMP_PSV_OFFSET	200
#endif

OperationRegion (SMNT, SystemMemory, CONFIG_ECAM_MMCONF_BASE_ADDRESS + 0x60, 0x8)
Field (SMNT, DWordAcc, NoLock, Preserve) {
	SMNI, 32, /* SMN Index 0 */
	SMND, 32  /* SMN Data 0 */
}

IndexField(SMNI, SMND, DWordAcc, NoLock, Preserve) {
	Offset (SMU_THERMAL_BLK_OFFSET),
	, 16,
	TJSE, 2,
	, 1,
	RSEL, 1,
	, 1,
	CTMP, 11,
	HTCE, 1,
	, 15,
	TLMT, 7 /* FIXME: What is the formula for HTC activation threshold? */
}

ThermalZone (TZ00) {
	Name (_STR, Unicode ("AMD CPU Core Thermal Sensor"))

	Method (_STA) {
		If (HTCE == 1) {
			Return (0x0F)
		}
		Return (0)
	}

	Method (_TMP) {	/* Current temp in tenths degree Kelvin. */
		Local0 = CTMP * 125 /* Temperature in 0.125 degrees unit, x1000 */
		If ((RSEL == 1) || (TJSE == 3)) {
			Local0 -= 49000 /* Start range is -49 degrees, x1000 */
		}
		/* Convert to Kelvin, x1000 */
		Local0 += (ACPI_TEMP_KELVIN_OFFSET * 100)
		/* Normalize to tenths of Kelvin as decribed in ACPI specification */
		Return (Local0 / 100)
	}

	Method (_PSV) {	/* Passive temp in tenths degree Kelvin. */
		Return (_CRT - CPU_TEMP_PSV_OFFSET)
	}

	Method (_HOT) {	/* Hot temp in tenths degree Kelvin. */
		Return (_CRT - CPU_TEMP_HOT_OFFSET)
	}

	Method (_CRT) {	/* Critical temp in tenths degree Kelvin. */
		Local0 = CPU_TEMP_CRIT_VALUE * 10
		Return (Local0 + ACPI_TEMP_KELVIN_OFFSET)
	}
}
