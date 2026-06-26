/* SPDX-License-Identifier: GPL-2.0-or-later */

Scope (\_SB)
{
	Name (TGPI, 0x0F)

	Device (ASMT)
	{
		Name (_HID, "ASMT0001")
		Name (_CID, "ASMT0001")
		Name (_UID, 0)
		Method (_CRS, 0, NotSerialized)
		{
			Name (RBUF, ResourceTemplate ()
			{
				GpioIo (Exclusive, PullUp, 0x0000, 0x0000, IoRestrictionNone,
					"\\_SB.PTIO", 0x00, ResourceConsumer, ,
					RawDataBuffer (0x01) { 0x01 })
					{
						0x0000
					}
				GpioIo (Exclusive, PullUp, 0x0000, 0x0000, IoRestrictionNone,
					"\\_SB.PTIO", 0x00, ResourceConsumer, ,
					RawDataBuffer (0x01) { 0x01 })
					{
						0x0001
					}
				GpioIo (Exclusive, PullUp, 0x0000, 0x0000, IoRestrictionNone,
					"\\_SB.PTIO", 0x00, ResourceConsumer, ,
					RawDataBuffer (0x01) { 0x01 })
					{
						0x0002
					}
				GpioIo (Exclusive, PullUp, 0x0000, 0x0000, IoRestrictionNone,
					"\\_SB.PTIO", 0x00, ResourceConsumer, ,
					RawDataBuffer (0x01) { 0x01 })
					{
						0x0003
					}
				GpioIo (Exclusive, PullUp, 0x0000, 0x0000, IoRestrictionNone,
					"\\_SB.PTIO", 0x00, ResourceConsumer, ,
					RawDataBuffer (0x01) { 0x01 })
					{
						0x0004
					}
				GpioIo (Exclusive, PullUp, 0x0000, 0x0000, IoRestrictionNone,
					"\\_SB.PTIO", 0x00, ResourceConsumer, ,
					RawDataBuffer (0x01) { 0x01 })
					{
						0x0005
					}
				GpioIo (Exclusive, PullUp, 0x0000, 0x0000, IoRestrictionNone,
					"\\_SB.PTIO", 0x00, ResourceConsumer, ,
					RawDataBuffer (0x01) { 0x01 })
					{
						0x0006
					}
				GpioIo (Exclusive, PullUp, 0x0000, 0x0000, IoRestrictionNone,
					"\\_SB.PTIO", 0x00, ResourceConsumer, ,
					RawDataBuffer (0x01) { 0x01 })
					{
						0x0007
					}
				GpioIo (Exclusive, PullUp, 0x0000, 0x0000, IoRestrictionNone,
					"\\_SB.PTIO", 0x00, ResourceConsumer, ,
					RawDataBuffer (0x01) { 0x01 })
					{
						0x0008
					}
				GpioIo (Exclusive, PullUp, 0x0000, 0x0000, IoRestrictionNone,
					"\\_SB.PTIO", 0x00, ResourceConsumer, ,
					RawDataBuffer (0x01) { 0x01 })
					{
						0x0009
					}
				GpioIo (Exclusive, PullUp, 0x0000, 0x0000, IoRestrictionNone,
					"\\_SB.PTIO", 0x00, ResourceConsumer, ,
					RawDataBuffer (0x01) { 0x01 })
					{
						0x000A
					}
				GpioIo (Exclusive, PullUp, 0x0000, 0x0000, IoRestrictionNone,
					"\\_SB.PTIO", 0x00, ResourceConsumer, ,
					RawDataBuffer (0x01) { 0x01 })
					{
						0x000B
					}
				GpioIo (Exclusive, PullUp, 0x0000, 0x0000, IoRestrictionNone,
					"\\_SB.PTIO", 0x00, ResourceConsumer, ,
					RawDataBuffer (0x01) { 0x01 })
					{
						0x000C
					}
				GpioIo (Exclusive, PullUp, 0x0000, 0x0000, IoRestrictionNone,
					"\\_SB.PTIO", 0x00, ResourceConsumer, ,
					RawDataBuffer (0x01) { 0x01 })
					{
						0x000D
					}
				GpioIo (Exclusive, PullUp, 0x0000, 0x0000, IoRestrictionNone,
					"\\_SB.PTIO", 0x00, ResourceConsumer, ,
					RawDataBuffer (0x01) { 0x01 })
					{
						0x000E
					}
				GpioIo (Exclusive, PullUp, 0x0000, 0x0000, IoRestrictionNone,
					"\\_SB.PTIO", 0x00, ResourceConsumer, ,
					RawDataBuffer (0x01) { 0x01 })
					{
						0x000F
					}
				GpioIo (Exclusive, PullUp, 0x0000, 0x0000, IoRestrictionNone,
					"\\_SB.PTIO", 0x00, ResourceConsumer, ,
					RawDataBuffer (0x01) { 0x01 })
					{
						0x0010
					}
				GpioIo (Exclusive, PullUp, 0x0000, 0x0000, IoRestrictionNone,
					"\\_SB.PTIO", 0x00, ResourceConsumer, ,
					RawDataBuffer (0x01) { 0x01 })
					{
						0x0011
					}
				GpioIo (Exclusive, PullUp, 0x0000, 0x0000, IoRestrictionNone,
					"\\_SB.PTIO", 0x00, ResourceConsumer, ,
					RawDataBuffer (0x01) { 0x01 })
					{
						0x0012
					}
				GpioIo (Exclusive, PullUp, 0x0000, 0x0000, IoRestrictionNone,
					"\\_SB.PTIO", 0x00, ResourceConsumer, ,
					RawDataBuffer (0x01) { 0x01 })
					{
						0x0013
					}
				GpioIo (Exclusive, PullUp, 0x0000, 0x0000, IoRestrictionNone,
					"\\_SB.PTIO", 0x00, ResourceConsumer, ,
					RawDataBuffer (0x01) { 0x01 })
					{
						0x0014
					}
				GpioIo (Exclusive, PullUp, 0x0000, 0x0000, IoRestrictionNone,
					"\\_SB.PTIO", 0x00, ResourceConsumer, ,
					RawDataBuffer (0x01) { 0x01 })
					{
						0x0015
					}
				GpioIo (Exclusive, PullUp, 0x0000, 0x0000, IoRestrictionNone,
					"\\_SB.PTIO", 0x00, ResourceConsumer, ,
					RawDataBuffer (0x01) { 0x01 })
					{
						0x0016
					}
				GpioIo (Exclusive, PullUp, 0x0000, 0x0000, IoRestrictionNone,
					"\\_SB.PTIO", 0x00, ResourceConsumer, ,
					RawDataBuffer (0x01) { 0x01 })
					{
						0x0017
					}
			})
			Return (RBUF)
		}

		Method (_STA, 0, NotSerialized)
		{
			If ((TGPI == One))
			{
				Return (0x0F)
			}
			Else
			{
				Return (Zero)
			}
		}
	}
}
