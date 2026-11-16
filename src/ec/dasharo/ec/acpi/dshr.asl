/* SPDX-License-Identifier: GPL-2.0-only */

#include "../commands.h"
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
			(1 << DASHARO_FEATURE_FAN_TACH) |
			(1 << DASHARO_FEATURE_FAN_MODE) |
			(1 << DASHARO_FEATURE_FAN_POINTS))
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

			// Same fans as DASHARO_FEATURE_FAN_PWM
			Case (DASHARO_FEATURE_FAN_MODE)
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

			Case (DASHARO_FEATURE_FAN_POINTS)
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

	// Get fan mode by cap and index: DASHARO_FAN_PWM_MODE_AUTO or _MANUAL
	Method (GFMD, 2, Serialized)
	{
		If (\_SB.PCI0.LPCB.EC0.ECOK == 0) { Return (DASHARO_FAN_PWM_MODE_AUTO) }

		Switch (ToInteger(Arg0))
		{
			Case (DASHARO_FAN_CPU)
			{
				If (Arg1 == 0) { Return (\_SB.PCI0.LPCB.EC0.FMN1) }
#if CONFIG_EC_DASHARO_EC_CPU_FAN_COUNT == 2
				If (Arg1 == 1) { Return (\_SB.PCI0.LPCB.EC0.FMN2) }
#endif
			}
#if CONFIG(EC_DASHARO_EC_DGPU)
			Case (DASHARO_FAN_GPU)
			{
				If (Arg1 == 0) { Return (\_SB.PCI0.LPCB.EC0.FMN2) }
			}
#endif
		}

		Return (DASHARO_FAN_PWM_MODE_AUTO)
	}

	// Set fan mode by cap and index. Returns 0 on success, 1 on failure.
	Method (SFMD, 3, Serialized)
	{
		If (\_SB.PCI0.LPCB.EC0.ECOK == 0) { Return (1) }

		Switch (ToInteger(Arg0))
		{
			Case (DASHARO_FAN_CPU)
			{
				If (Arg1 == 0)
				{
					\_SB.PCI0.LPCB.EC0.FMN1 = ToInteger(Arg2)
					Return (0)
				}
#if CONFIG_EC_DASHARO_EC_CPU_FAN_COUNT == 2
				If (Arg1 == 1)
				{
					\_SB.PCI0.LPCB.EC0.FMN2 = ToInteger(Arg2)
					Return (0)
				}
#endif
			}
#if CONFIG(EC_DASHARO_EC_DGPU)
			Case (DASHARO_FAN_GPU)
			{
				If (Arg1 == 0)
				{
					\_SB.PCI0.LPCB.EC0.FMN2 = ToInteger(Arg2)
					Return (0)
				}
			}
#endif
		}

		Return (1)
	}

	// Set fan duty cycle (0-255) by cap and index. Only accepted in manual
	// mode, the EC ignores duty writes otherwise. Returns 0 on success,
	// 1 on failure.
	Method (SFDC, 3, Serialized)
	{
		If (\_SB.PCI0.LPCB.EC0.ECOK == 0) { Return (1) }
		If (GFMD (Arg0, Arg1) == DASHARO_FAN_PWM_MODE_AUTO) { Return (1) }

		Switch (ToInteger(Arg0))
		{
			Case (DASHARO_FAN_CPU)
			{
				If (Arg1 == 0)
				{
					\_SB.PCI0.LPCB.EC0.DUT1 = ToInteger(Arg2)
					Return (0)
				}
#if CONFIG_EC_DASHARO_EC_CPU_FAN_COUNT == 2
				If (Arg1 == 1)
				{
					\_SB.PCI0.LPCB.EC0.DUT2 = ToInteger(Arg2)
					Return (0)
				}
#endif
			}
#if CONFIG(EC_DASHARO_EC_DGPU)
			Case (DASHARO_FAN_GPU)
			{
				If (Arg1 == 0)
				{
					\_SB.PCI0.LPCB.EC0.DUT2 = ToInteger(Arg2)
					Return (0)
				}
			}
#endif
		}

		Return (1)
	}

	OperationRegion (SMFI, SystemIO, 0x0E00, 0x100)
	Field (SMFI, ByteAcc, NoLock, Preserve)
	{
		SCMD, 8,
		SRES, 8,
		// 254 bytes of data
		SDAT, 2032
	}

	// writes to SMFI will be done multiple times and will sleep to wait for EC
	// responses, so we need a lock to prevent clashes
	Mutex (SMFM, 0)

	// SMFI Wait, wait until EC is ready for next command
	Method (SMFW, 0, Serialized)
	{
		Local0 = 10
		While ((SCMD != CMD_NONE) && (Local0 > 0))
		{
			Sleep (1)
			Local0--
		}

		Return (SCMD != CMD_NONE)
	}

	// SMFI Command, send
	// Arg0 - command number, Arg1 - buffer with the command data
	// Returns the EC result (0 on success), 1 on timeout
	Method (SMFC, 2, Serialized)
	{
		If (SMFW ()) { Return (1) }

		SDAT = Arg1
		SCMD = Arg0

		If (SMFW ()) { Return (1) }

		Return (SRES)
	}

	// Fan Curve Fan Number
	// Arg0 - cap of the fan, Arg1 - index of the fan
	// Returns FAN_CURVE_FAN1 or FAN_CURVE_FAN2, 0xFF if there is no such fan
	Method (FCFN, 2, Serialized)
	{
		Switch (ToInteger (Arg0))
		{
			Case (DASHARO_FAN_CPU)
			{
				If (Arg1 == 0) { Return (FAN_CURVE_FAN1) }
#if CONFIG_EC_DASHARO_EC_CPU_FAN_COUNT == 2
				If (Arg1 == 1) { Return (FAN_CURVE_FAN2) }
#endif
			}
#if CONFIG(EC_DASHARO_EC_DGPU)
			Case (DASHARO_FAN_GPU)
			{
				If (Arg1 == 0) { Return (FAN_CURVE_FAN2) }
			}
#endif
		}

		Return (0xFF)
	}

	// Fan Curve Read
	// Arg0 - fan number, Arg1 - curve to read (FAN_CURVE_APPLIED or
	// FAN_CURVE_PENDING)
	// Returns a buffer: fan number, point count, then temp and duty of each point.
	// The count is 0 on failure.
	Method (FCRD, 2, Serialized)
	{
		Local0 = Buffer (2) { 0, 0 }
		Local0[0] = Arg0
		Local0[1] = Arg1

		If (SMFC (CMD_FAN_CURVE_GET, Local0)) { Return (Buffer (2) { 0, 0 }) }

		Local1 = SDAT
		Return (Local1)
	}

	// Fan Curve Get Point
	// Arg0 - fan number, Arg1 - point index, Arg2 - field (0 temp, 1 duty)
	Method (FCGP, 3, Serialized)
	{
		If (Arg0 == 0xFF) { Return (0) }

		Acquire (SMFM, 0xFFFF)
		Local0 = FCRD (Arg0, FAN_CURVE_APPLIED)
		Release (SMFM)

		If (Arg1 >= DerefOf (Local0[1])) { Return (0) }

		Return (DerefOf (Local0[2 + 2 * Arg1 + Arg2]))
	}

	// Fan Curve Set Point
	// Arg0 - fan number, Arg1 - point idx,
	// Arg2 - field of the point (0 temp, 1 duty),
	// Arg3 - value to set (degrees C or duty in percent)
	Method (FCSP, 4, Serialized)
	{
		If (Arg0 == 0xFF) { Return (1) }

		Acquire (SMFM, 0xFFFF)
		Local0 = FCRD (Arg0, FAN_CURVE_PENDING)

		If (Arg1 >= DerefOf (Local0[1]))
		{
			Release (SMFM)
			Return (1)
		}

		Local0[2 + 2 * Arg1 + Arg2] = Arg3
		Local1 = SMFC (CMD_FAN_CURVE_PENDING_SET, Local0)
		Release (SMFM)

		Return (Local1)
	}

	// Get Fan curve Point Temp, degrees C
	Method (GFPT, 3, Serialized)
	{
		If (\_SB.PCI0.LPCB.EC0.ECOK == 0) { Return (0) }

		Return (FCGP (FCFN (Arg0, Arg1), ToInteger (Arg2), 0))
	}

	// Get Fan Point Duty in percent
	Method (GFPD, 3, Serialized)
	{
		If (\_SB.PCI0.LPCB.EC0.ECOK == 0) { Return (0) }

		Return (FCGP (FCFN (Arg0, Arg1), ToInteger (Arg2), 1))
	}

	// Set Fan Point Temp in degrees C (Arg3). Takes effect at the next
	// CFCV. Returns 0 on success, 1 on failure.
	Method (SFPT, 4, Serialized)
	{
		If (\_SB.PCI0.LPCB.EC0.ECOK == 0) { Return (1) }

		Return (FCSP (FCFN (Arg0, Arg1), ToInteger (Arg2), 0, ToInteger (Arg3)))
	}

	// Set Fan Point Duty in percent (Arg3). Takes effect at the next CFCV.
	// Returns 0 on success, 1 on failure.
	Method (SFPD, 4, Serialized)
	{
		If (\_SB.PCI0.LPCB.EC0.ECOK == 0) { Return (1) }

		Return (FCSP (FCFN (Arg0, Arg1), ToInteger (Arg2), 1, ToInteger (Arg3)))
	}

	// Commit the Fan Curve points of one fan.
	// The EC validates the curve and applies it.
	Method (CFCV, 2, Serialized)
	{
		If (\_SB.PCI0.LPCB.EC0.ECOK == 0) { Return (1) }

		Local0 = Buffer (1) { 0 }
		Local0[0] = FCFN (Arg0, Arg1)
		If (DerefOf (Local0[0]) == 0xFF) { Return (1) }

		Acquire (SMFM, 0xFFFF)
		Local1 = SMFC (CMD_FAN_CURVE_COMMIT, Local0)
		Release (SMFM)

		Return (Local1)
	}

	// APM command port, writes raise an SMI handled by coreboot
	OperationRegion (APMP, SystemIO, 0xB2, 1)
	Field (APMP, ByteAcc, NoLock, Preserve)
	{
		APMC, 8
	}

	// Save Fan Curve
	Method (SFCS, 2, Serialized)
	{
		If (\_SB.PCI0.LPCB.EC0.ECOK == 0) { Return (1) }

		Acquire (SMFM, 0xFFFF)
		APMC = DASHARO_APM_FAN_CURVE_SAVE
		Release (SMFM)

		Return (0)
	}
}
