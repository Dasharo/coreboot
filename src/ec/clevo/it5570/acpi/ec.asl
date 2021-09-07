/* SPDX-License-Identifier: GPL-2.0-only */

Device (EC0)
{
	Name (_HID, EisaId ("PNP0C09") /* Embedded Controller Device */)  // _HID: Hardware ID
	Name (_GPE, EC_GPE_SCI)  // _GPE: General Purpose Events
	Name (_CRS, ResourceTemplate ()  // _CRS: Current Resource Settings
	{
		IO (Decode16,
			0x0062,             // Range Minimum
			0x0062,             // Range Maximum
			0x00,               // Alignment
			0x01,               // Length
			)
		IO (Decode16,
			0x0066,             // Range Minimum
			0x0066,             // Range Maximum
			0x00,               // Alignment
			0x01,               // Length
			)
		IO (Decode16,
			0x002e,             // Range Minimum
			0x002e,             // Range Maximum
			0x00,               // Alignment
			0x02,               // Length
			)
	})

	#include "ec_ram.asl"

	Name (ECOK, Zero)
	Method (_REG, 2, Serialized)  // _REG: Region Availability
	{
		Debug = Concatenate("EC: _REG", Concatenate(ToHexString(Arg0), Concatenate(" ", ToHexString(Arg1))))
		If (((Arg0 == 0x03) && (Arg1 == One))) {
			// Enable hardware touchpad lock and keyboard backlight keys
			// Enable software airplane mode key
			ECOS = 2

			// Enable software display brightness keys
			WINF = 1

			// Enable camera toggle hotkey
			OEM3 = OEM3 | 4

			// Set current AC state
			\_SB.AC.ACFG = ADP
			// Update battery information and status
			\_SB.BAT0.UPBI ()
			\_SB.BAT0.UPBS ()

			PNOT ()

			// Initialize UCSI
			^UCSI.INIT ()

			// EC is now available
			ECOK = Arg1
		}
	}

	Method (PTS, 1, Serialized) {
		Debug = Concatenate("EC: PTS: ", ToHexString(Arg0))
		If (ECOK) {
			// Clear wake cause
			WFNO = Zero
		}
	}

	Method (WAK, 1, Serialized) {
		Debug = Concatenate("EC: WAK: ", ToHexString(Arg0))
		If (ECOK) {
			// Set current AC state
			\_SB.AC.ACFG = ADP

			// Update battery information and status
			\_SB.BAT0.UPBI ()
			\_SB.BAT0.UPBS ()

			// Notify of changes
			Notify(\_SB.AC, Zero)
			Notify(\_SB.BAT0, Zero)

			Sleep (1000)
		}
	}

	Method (I2ER, 1, NotSerialized)
	{
		SIOI = 0x2e
		SIOD = 0x11
		SIOI = 0x2f
		SIOD = ((Arg0 >> 8) & 0xff)
		SIOI = 0x2e
		SIOD = 0x10
		SIOI = 0x2f
		SIOD = (Arg0 & 0xff)
		SIOI = 0x2e
		SIOD = 0x12
		SIOI = 0x2f
		Local0 = SIOD

		Return (Local0)
	}

	Method (I2EW, 2, NotSerialized)
	{
		SIOI = 0x2e
		SIOD = 0x11
		SIOI = 0x2f
		SIOD = ((Arg0 >> 8) & 0xff)
		SIOI = 0x2e
		SIOD = 0x10
		SIOI = 0x2f
		SIOD = (Arg0 & 0xff)
		SIOI = 0x2e
		SIOD = 0x12
		SIOI = 0x2f

		SIOD = Arg1;
	}

	Method (GKBL, 0, Serialized) // Get Keyboard LED
	{
		Local0 = 0
		If (ECOK) {
			FDAT = One
			FCMD = 0xCA
			Local0 = FBUF
			FCMD = Zero
		}
		Return (Local0)
	}

	Method (SKBL, 1, Serialized) // Set Keyboard LED
	{
		If (ECOK) {
			FDAT = Zero
			FBUF = Arg0
			FCMD = 0xCA
		}
	}

	Method (_Q0A, 0, NotSerialized) // Touchpad Toggle
	{
		Debug = "EC: Touchpad Toggle"
	}

	Method (_Q0B, 0, NotSerialized) // Screen Toggle
	{
		Debug = "EC: Screen Toggle"
	}

	Method (_Q0C, 0, NotSerialized)  // Mute
	{
		Debug = "EC: Mute"
	}

	Method (_Q0D, 0, NotSerialized) // Keyboard Backlight
	{
		Debug = "EC: Keyboard Backlight"
	}

	Method (_Q0E, 0, NotSerialized) // Volume Down
	{
		Debug = "EC: Volume Down"
	}

	Method (_Q0F, 0, NotSerialized) // Volume Up
	{
		Debug = "EC: Volume Up"
	}

	Method (_Q10, 0, NotSerialized) // Switch Video Mode
	{
		Debug = "EC: Switch Video Mode"
	}

	Method (_Q11, 0, NotSerialized) // Brightness Down
	{
		Debug = "EC: Brightness Down"

		If (CondRefOf (\_SB.PCI0.GFX0.LCD0)) {
			Notify (\_SB.PCI0.GFX0.LCD0, 0x87)
		}
	}

	Method (_Q12, 0, NotSerialized) // Brightness Up
	{
		Debug = "EC: Brightness Up"

		If (CondRefOf (\_SB.PCI0.GFX0.LCD0)) {
			Notify (\_SB.PCI0.GFX0.LCD0, 0x86)
		}
	}

	Method (_Q13, 0, NotSerialized) // Camera Toggle
	{
		Debug = "EC: Camera Toggle"
	}

	Method (_Q14, 0, NotSerialized) // Airplane Mode
	{
		Debug = "EC: Airplane Mode"

		If (CondRefOf (^HIDD.HPEM))
		{
			^HIDD.HPEM (8)
		}
	}

	Method (_Q15, 0, NotSerialized) // Suspend Button
	{
		Debug = "EC: Suspend Button"
		Notify (SLPB, 0x80)
	}

	Method (_Q16, 0, NotSerialized) // AC Detect
	{
		Debug = "EC: AC Detect"
		\_SB.AC.ACFG = ADP
		Notify (AC, 0x80) // Status Change
		Sleep (0x01F4)
		If (BAT0)
		{
			Notify (\_SB.BAT0, 0x81) // Information Change
			Sleep (0x32)
			Notify (\_SB.BAT0, 0x80) // Status Change
			Sleep (0x32)
		}
	}

	Method (_Q17, 0, NotSerialized)  // BAT0 Update
	{
		Debug = "EC: BAT0 Update (17)"
		Notify (\_SB.BAT0, 0x81) // Information Change
	}

	Method (_Q19, 0, NotSerialized)  // BAT0 Update
	{
		Debug = "EC: BAT0 Update (19)"
		Notify (\_SB.BAT0, 0x81) // Information Change
	}

	Method (_Q1B, 0, NotSerialized) // Lid Close
	{
		Debug = "EC: Lid Close"
		Notify (LID0, 0x80)
	}

	Method (_Q1C, 0, NotSerialized) // Thermal Trip
	{
		Debug = "EC: Thermal Trip"
		/* TODO
		Notify (\_TZ.TZ0, 0x81) // Thermal Trip Point Change
		Notify (\_TZ.TZ0, 0x80) // Thermal Status Change
		*/
	}

	Method (_Q1D, 0, NotSerialized) // Power Button
	{
		Debug = "EC: Power Button"
		Notify (PWRB, 0x80)
	}

	Method (_Q50, 0, NotSerialized) // Other Events
	{
		Local0 = OEM4
		If (Local0 == 0x8A) {
			Debug = "EC: White Keyboard Backlight"
		} ElseIf (Local0 == 0x9F) {
			Debug = "EC: Color Keyboard Toggle"
		} ElseIf (Local0 == 0x81) {
			Debug = "EC: Color Keyboard Down"
		} ElseIf (Local0 == 0x82) {
			Debug = "EC: Color Keyboard Up"
		} ElseIf (Local0 == 0x80) {
			Debug = "EC: Color Keyboard Color Change"
		} Else {
			Debug = Concatenate("EC: Other: ", ToHexString(Local0))
		}
	}

	Method (_Q62, 0, NotSerialized)  // UCSI event
	{
		Debug = "EC: UCSI Event"
		^UCSI.MGI0 = MGI0
		^UCSI.MGI1 = MGI1
		^UCSI.MGI2 = MGI2
		^UCSI.MGI3 = MGI3
		^UCSI.MGI4 = MGI4
		^UCSI.MGI5 = MGI5
		^UCSI.MGI6 = MGI6
		^UCSI.MGI7 = MGI7
		^UCSI.MGI8 = MGI8
		^UCSI.MGI9 = MGI9
		^UCSI.MGIA = MGIA
		^UCSI.MGIB = MGIB
		^UCSI.MGIC = MGIC
		^UCSI.MGID = MGID
		^UCSI.MGIE = MGIE
		^UCSI.MGIF = MGIF
		^UCSI.CCI0 = CCI0
		^UCSI.CCI1 = CCI1
		^UCSI.CCI2 = CCI2
		^UCSI.CCI3 = CCI3
		CCI0 = Zero
		CCI3 = Zero
		Notify (^UCSI, 0x80)
	}

	#include "hid.asl"
	#include "ucsi.asl"
}
