/* We don't have a real SIO but lets define the serial port here, this is where it belongs */

	Device (COM1) {
		Name (_HID, EISAID ("PNP0501"))
		Name (_UID, 1)
		Name (_ADR, 0)

		Method (_STA, 0, NotSerialized) {
			Return (0x0F)
		}

		Name (_CRS, ResourceTemplate ()
		{
			IO (Decode16, 0x03F8, 0x3F8, 0x08, 0x08)
			IRQNoFlags () {4}
//			IRQNoFlags () {}
		})

		Name (_PRS, ResourceTemplate ()
		{
			StartDependentFn (0, 0) {
				IO (Decode16, 0x03F8, 0x3F8, 0x08, 0x08)
//				IRQNoFlags () {4}
				IRQNoFlags () {}
			}
			EndDependentFn ()
		})
	}



