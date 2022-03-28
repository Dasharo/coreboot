/* SPDX-License-Identifier: GPL-2.0-only */

Name (KBEN, 0x01) // Keyboard Backlight Enable
Name (KBBR, 0xFF) // Keyboard Brightness
Name (KBCR, 0xFF) // Keyboard Color Red
Name (KBCG, 0xFF) // Keyboard Color Green
Name (KBCB, 0xFF) // Keyboard Color Blue
Name (KBPS, 0x00) // Keyboard Preset

Method (KBTG, 0, NotSerialized) // Keyboard Toggle
{
	KBEN ^= 1
	KBUP ()
}

Method (KBBU, 0, NotSerialized) // Keyboard Brightness Up
{
	If (KBBR <= 0xDF) {
		KBBR += 0x20
	}
	KBUP()
}

Method (KBBD, 0, NotSerialized) // Keyboard Brightness Down
{
	If (KBBR >= 0x2F) {
		KBBR -= 0x20
	}
	KBUP()
}

Method (KBPN, 0, NotSerialized) // Keyboard Preset Next
{
	KBPS = (KBPS + 1) % 7

	If (KBPS == 0x00) { // White
		KBCR = 0xFF
		KBCG = 0xFF
		KBCG = 0xFF
	} ElseIf (KBPS == 0x01) { // Red
		KBCR = 0xFF
		KBCG = 0x00
		KBCB = 0x00
	} ElseIf (KBPS == 0x02) { // Green
		KBCR = 0x00
		KBCG = 0xFF
		KBCB = 0x00
	} ElseIf (KBPS == 0x03) { // Blue
		KBCR = 0x00
		KBCG = 0x00
		KBCB = 0xFF
	} ElseIf (KBPS == 0x04) { // Yellow
		KBCR = 0xFF
		KBCG = 0xFF
		KBCB = 0x00
	} ElseIf (KBPS == 0x05) { // Magenta
		KBCR = 0xFF
		KBCG = 0x00
		KBCB = 0xFF
	} ElseIf (KBPS == 0x06) { // Cyan
		KBCR = 0x00
		KBCG = 0xFF
		KBCB = 0xFF
	}

	KBUP ()
}

Method (KBUP, 0, NotSerialized) // Keyboard Update
{
	Debug = "EC: Keyboard Update"

	// Left zone colors
	FDAT = 0x03
	FBUF = KBCB
	FBF1 = KBCR
	FBF2 = KBCG
	FCMD = 0xCA

	// Middle zone colors
	FDAT = 0x04
	FBUF = KBCB
	FBF1 = KBCR
	FBF2 = KBCG
	FCMD = 0xCA

	// Right zone colors
	FDAT = 0x05
	FBUF = KBCB
	FBF1 = KBCR
	FBF2 = KBCG
	FCMD = 0xCA

	// Brightness
	FDAT = 0x06
	FBUF = KBBR
	FCMD = 0xCA

	// Enable
	FDAT = 0x0C
	If (KBEN & LSTE) { // Always disable when lid is closed
		FBUF = 0x3F
	} Else {
		FBUF = 0x20
	}
	FCMD = 0xC4
}
