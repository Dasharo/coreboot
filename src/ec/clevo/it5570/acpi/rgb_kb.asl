Name (KBEN, 0x01) // Keyboard Backlight Enable
Name (KBBR, 0xFF) // Keyboard Brightness
Name (KBCR, 0xFF) // Keyboard Color Red
Name (KBCG, 0xFF) // Keyboard Color Green
Name (KBCB, 0xFF) // Keyboard Color Blue

Method (KBTG, 0, NotSerialized) // Keyboard Toggle
{
	KBEN ^= 1
	KBUP ()
}

Method (KBBU, 0, NotSerialized) // Keyboard Brightness Up
{
	If (KBBR <= 0xEF) {
		KBBR += 0x10
	}
	KBUP()
}

Method (KBBD, 0, NotSerialized) // Keyboard Brightness Down
{
	If (KBBR >= 0x1f) {
		KBBR -= 0x10
	}
	KBUP()
}

Method (KBPN, 0, NotSerialized) // Keyboard Preset Next
{
	// TODO
	KBUP ()
}

Method (KBUP, 0, NotSerialized) // Keyboard Update
{
	// Left zone colors
	FDAT = 0x03
	FBUF = KBCB
	FBF1 = KBCG
	FBF2 = KBCR
	FCMD = 0xCA

	// Middle zone colors
	FDAT = 0x04
	FBUF = KBCB
	FBF1 = KBCG
	FBF2 = KBCR
	FCMD = 0xCA

	// Right zone colors
	FDAT = 0x05
	FBUF = KBCB
	FBF1 = KBCG
	FBF2 = KBCR
	FCMD = 0xCA

	// Brightness
	FDAT = 0x06
	FBUF = KBBR
	FCMD = 0xCA

	// Enable
	FDAT = 0x0C
	If (KBEN) {
		FBUF = 0x3F
	} Else {
		FBUF = 0x20
	}
	FCMD = 0xC4
}
