Scope (\_TZ)
{
	Device (TFN1)
	{
		Name (_HID, EISAID ("PNP0C0B"))
		Name (_UID, 0)
		Name (_STR, Unicode("CPU Fan"))

		/* _FIF: Fan Information */
		Name (_FIF, Package ()
		{
			0,	// Revision
			1,	// Fine Grained Control
			1,	// Step Size
			0	// No Low Speed Notification
		})

		Name (TFST, Package ()
		{
			0,	// Revision
			0x00,	// Control
			0x00	// Speed
		})

		/* _FST: Fan current Status */
		Method (_FST, 0, Serialized,,PkgObj)
		{
			/* Fill in TFST with current control. */
			TFST [1] = (\_SB.S76D.GFAN(0) & 0xFF) * 100 / 255
			TFST [2] = \_SB.S76D.GFAN(0) >> 8
			Return (TFST)
		}

		/* _FPS: Fan Performance States */
		Method (_FPS, 0, Serialized) {
			Return (Package() {
				0,	// Revision
				/*
				 * TODO : Need to update this Table after characterization.
				 * These are just the 2 extremes and only the required
				 * params are given.
				 */
				/* Control, Trip Point, Speed, NoiseLevel, Power */
				Package () {100,	0xFFFFFFFF,	6000,	0xFFFFFFFF,	0xFFFFFFFF},
				Package () {0,		0xFFFFFFFF,	0,		0xFFFFFFFF,	0xFFFFFFFF},
			})
		}

		/* _FSL: Fan Set Level */
		Method (_FSL, 1, Serialized)
		{
			/* Not supported */
		}
	}

	ThermalZone (TZ00)
	{
		/* _TMP: Temperature */
		Method (_TMP, 0, Serialized)
		{
			Local0 = 0
			If (\_SB.PCI0.LPCB.EC0.ECOK) {
				Local0 = (0x0AAC + (\_SB.PCI0.LPCB.EC0.TMP1 * 0x0A))
			}
			Return (Local0)
		}

		/* _CRT: Critical Temperature */
		Method (_CRT, 0, Serialized)
		{
			/* defined in board mainboard.asl */
			Return (0x0AAC + (CPU_CRIT_TEMP * 0x0A))
		}

		Method (_AC0) {
			Return (0)
		}

		Name (_AL0, Package () { TFN1 })
	}

#if CONFIG(EC_SYSTEM76_EC_DGPU)
	Device (TFN2)
	{
		Name (_HID, EISAID ("PNP0C0B"))
		Name (_UID, 1)
		Name (_STR, Unicode("GPU Fan"))

		/* _FIF: Fan Information */
		Name (_FIF, Package ()
		{
			0,	// Revision
			1,	// Fine Grained Control
			1,	// Step Size
			0	// No Low Speed Notification
		})

		Name (TFST, Package ()
		{
			0,	// Revision
			0x00,	// Control
			0x00	// Speed
		})

		/* _FST: Fan current Status */
		Method (_FST, 0, Serialized,,PkgObj)
		{
			/* Fill in TFST with current control. */
			TFST [1] = (\_SB.S76D.GFAN(1) & 0xFF) * 100 / 255
			TFST [2] = \_SB.S76D.GFAN(1) >> 8
			Return (TFST)
		}

		/* _FPS: Fan Performance States */
		Method (_FPS, 0, Serialized) {
			Return (Package() {
				0,	// Revision
				/*
				 * TODO : Need to update this Table after characterization.
				 * These are just the 2 extremes and only the required
				 * params are given.
				 */
				/* Control, Trip Point, Speed, NoiseLevel, Power */
				Package () {100,	0xFFFFFFFF,	6000,	0xFFFFFFFF,	0xFFFFFFFF},
				Package () {0,		0xFFFFFFFF,	0,		0xFFFFFFFF,	0xFFFFFFFF},
			})
		}

		/* _FSL: Fan Set Level */
		Method (_FSL, 1, Serialized)
		{
			/* Not supported */
		}
	}

	ThermalZone (TZ01)
	{
		/* _TMP: Temperature */
		Method (_TMP, 0, Serialized)
		{
			Local0 = 0
			If (\_SB.PCI0.LPCB.EC0.ECOK) {
				Local0 = (0x0AAC + (\_SB.PCI0.LPCB.EC0.TMP2 * 0x0A))
			}
			Return (Local0)
		}

		/* _CRT: Critical Temperature */
		Method (_CRT, 0, Serialized)
		{
			/* defined in board mainboard.asl */
			Return (0x0AAC + (GPU_CRIT_TEMP * 0x0A))
		}

		Method (_AC0) {
			Return (0)
		}

		Name (_AL0, Package () { TFN2 })
	}
#endif
}
