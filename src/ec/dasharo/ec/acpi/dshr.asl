/* SPDX-License-Identifier: GPL-2.0-only */

#include "../driver.h"

Device (DASH) {
	Name (_HID, "DSHR0001")
	Name (_UID, 0)
	// Hide the device so that Windows does not warn about a missing driver.
	Name (_STA, 0xB)

	// Version of the ACPI interface
	Name (DSVR, 1)

	// Get platform features
	Method (GFTR, 0, Serialized)
	{
		Return (
			(1 << DASHARO_FEATURE_TEMPERATURE) |
			(1 << DASHARO_FEATURE_FAN_PWM) |
			(1 << DASHARO_FEATURE_FAN_TACH))
	}

	// Get feature capabilities
	Method (GFCP, 2, Serialized)
	{
		Switch (ToInteger(Arg0))
		{
			Case (DASHARO_FEATURE_TEMPERATURE)
			{
				Switch (ToInteger(Arg1))
				{
					Case (DASHARO_TEMPERATURE_CPU_PACKAGE) { Return (1) }
#if CONFIG(EC_DASHARO_EC_DGPU)
					Case (DASHARO_TEMPERATURE_GPU) { Return (1) }
#endif
					Default { Return (0) }
				}
			}

			Case (DASHARO_FEATURE_FAN_PWM)
			{
				Switch (ToInteger(Arg1))
				{
#if CONFIG(EC_DASHARO_EC_DGPU)
					Case (DASHARO_FAN_CPU) { Return (1) }
					Case (DASHARO_FAN_GPU) { Return (1) }
#else
					Case (DASHARO_FAN_CPU) { Return (CONFIG_EC_DASHARO_EC_CPU_FAN_COUNT) }
#endif
					Default { Return (0) }
				}
			}

			Case (DASHARO_FEATURE_FAN_TACH)
			{
				Switch (ToInteger(Arg1))
				{
#if CONFIG(EC_DASHARO_EC_DGPU)
					Case (DASHARO_FAN_CPU) { Return (1) }
					Case (DASHARO_FAN_GPU) { Return (1) }
#else
					Case (DASHARO_FAN_CPU) { Return (CONFIG_EC_DASHARO_EC_CPU_FAN_COUNT) }
#endif
					Default { Return (0) }
				}
			}
			Default { Return (0) }
		}
	}

	// Get temperature by capability and index
	Method (GTMP, 2, Serialized)
	{
		If (\_SB.PCI0.LPCB.EC0.ECOK == 0) { Return (0) }

		Switch (ToInteger(Arg0))
		{
			Case (DASHARO_TEMPERATURE_CPU_PACKAGE)
			{
				If (Arg1 == 0) { Return (\_SB.PCI0.LPCB.EC0.TMP1) }
			}
#if CONFIG(EC_DASHARO_EC_DGPU)
			Case (DASHARO_TEMPERATURE_GPU)
			{
				If (Arg1 == 0) { Return (\_SB.PCI0.LPCB.EC0.TMP2) }
			}
#endif
			Default { Return (0) }
		}

		Return (0)
	}

	// Get fan tach reading by cap and index
	Method (GFTH, 2, Serialized)
	{
		If (\_SB.PCI0.LPCB.EC0.ECOK == 0) { Return (0) }

		Switch (ToInteger(Arg0))
		{
			// FIXME: Some laptops have dual CPU fans with separate tach,
			// while some share a single tach
			Case (DASHARO_TEMPERATURE_CPU_PACKAGE)
			{
				If (Arg1 == 0)
				{
					Local0 = \_SB.PCI0.LPCB.EC0.RPM1
					If (Local0 == 0) { Return (0) }

					// 60 * (EC frequency (9.2MHz) / 128) / 2
					Return (2156250 / Local0)
				}
#if CONFIG_EC_DASHARO_EC_CPU_FAN_COUNT == 2
				If (Arg1 == 1)
				{
					Local0 = \_SB.PCI0.LPCB.EC0.RPM2
					If (Local0 == 0) { Return (0) }

					// 60 * (EC frequency (9.2MHz) / 128) / 2
					Return (2156250 / Local0)
				}
#endif
			}
#if CONFIG(EC_DASHARO_EC_DGPU)
			Case (DASHARO_FAN_GPU)
			{
				If (Arg1 == 0)
				{
					Local0 = \_SB.PCI0.LPCB.EC0.RPM2
					If (Local0 == 0) { Return (0) }

					// 60 * (EC frequency (9.2MHz) / 128) / 2
					Return (2156250 / Local0)
				}
			}
#endif
			Default { Return (0) }
		}

		Return (0)
	}

	// Get fan duty cycle by cap and index
	Method (GFDC, 2, Serialized)
	{
		If (\_SB.PCI0.LPCB.EC0.ECOK == 0) { Return (0) }

		Switch (ToInteger(Arg0))
		{
			Case (DASHARO_TEMPERATURE_CPU_PACKAGE)
			{
				If (Arg1 == 0) { Return (\_SB.PCI0.LPCB.EC0.DUT1) }
#if CONFIG_EC_DASHARO_EC_CPU_FAN_COUNT == 2
				If (Arg1 == 1) { Return (\_SB.PCI0.LPCB.EC0.DUT2) }
#endif
			}
#if CONFIG(EC_DASHARO_EC_DGPU)
			Case (DASHARO_FAN_GPU)
			{
				If (Arg1 == 0) { Return (\_SB.PCI0.LPCB.EC0.DUT2) }
			}
#endif
			Default { Return (0) }
		}

		Return (0)
	}
}
