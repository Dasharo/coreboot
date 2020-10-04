/* SPDX-License-Identifier: GPL-2.0-only */

Scope (\_SB.PCI0.IPU0)
{
	Name (_DSD, Package (0x02)  /* _DSD: Device-Specific Data */
	{
		ToUUID ("dbb8e3e6-5886-4ba6-8795-1319f52a966b"),
		Package (0x02)
		{
			Package (0x02)
			{
				"port0",
				"PRT0"
			},
			Package (0x02)
			{
				"port1",
				"PRT1"
			}
		}
	})

	Name (PRT0, Package (0x04)
	{
		ToUUID ("daffd814-6eba-4d8c-8a91-bc9bbf4aa301"),
		Package (0x01)
		{
			Package (0x02)
			{
				"port",
				5
			}
		},
		ToUUID ("dbb8e3e6-5886-4ba6-8795-1319f52a966b"),
		Package (0x01)
		{
			Package (0x02)
			{
				"endpoint0",
				"EP00"
			}
		}
	})

	Name (PRT1, Package (0x04)
	{
		ToUUID ("daffd814-6eba-4d8c-8a91-bc9bbf4aa301"),
		Package (0x01)
		{
			Package (0x02)
			{
				"port",
				1
			}
		},

		ToUUID ("dbb8e3e6-5886-4ba6-8795-1319f52a966b"),
		Package (0x01)
		{
			Package (0x02)
			{
				"endpoint0",
				"EP10"
			}
		}
	})
}

Scope (\_SB.PCI0.IPU0)
{
	Name (EP00, Package (0x02)
	{
		ToUUID ("daffd814-6eba-4d8c-8a91-bc9bbf4aa301"),
		Package (0x04)
		{
			Package (0x02)
			{
				"endpoint",
				Zero
			},
			Package (0x02)
			{
				"clock-lanes",
				Zero
			},
			Package (0x02)
			{
				"data-lanes",
				Package (0x04)
				{
					One,
					0x02,
					0x03,
					0x04
				}
			},
			Package (0x02)
			{
				"remote-endpoint",
				Package (0x03)
				{
					^I2C3.CAM0,
					Zero,
					Zero
				}
			}
		}
	})
	Name (EP10, Package (0x02)
	{
		ToUUID ("daffd814-6eba-4d8c-8a91-bc9bbf4aa301"),
		Package (0x04)
		{
			Package (0x02)
			{
				"endpoint",
				Zero
			},
			Package (0x02)
			{
				"clock-lanes",
				Zero
			},
			Package (0x02)
			{
				"data-lanes",
				Package (0x02)
				{
					One,
					0x02
				}
			},
			Package (0x02)
			{
				"remote-endpoint",
				Package (0x03)
				{
					^I2C2.CAM1,
					Zero,
					Zero
				}
			}
		}
	})
}

Scope (\_SB.PCI0.I2C3)
{
	PowerResource (RCPR, 0x00, 0x0000)
	{
		Name (STA, Zero)
		Method (_ON, 0, Serialized)  /* Rear camera_ON_: Power On */
		{
			If ((STA == Zero))
			{
				/* Enable IMG_CLK */
				MCON(3,1) /* Clock 3, 19.2MHz */

				/* Pull RST low */
#if CONFIG(BOARD_GOOGLE_VOLTEER)
				CTXS(GPP_F15)
#else
				CTXS(GPP_D4)
#endif

				/* Pull SNRPWR_EN high */
				STXS(GPP_H14)

				/* Pull PWREN high */
				STXS(GPP_H20)
				Sleep(2) /* reset pulse width */

				/* Pull RST high */
#if CONFIG(BOARD_GOOGLE_VOLTEER)
				STXS(GPP_F15)
#else
				STXS(GPP_D4)
#endif
				Sleep(1) /* t2 */

				Store(1,STA)
			}
		}
		Method (_OFF, 0, Serialized)  /* Rear camera _OFF: Power Off */
		{
			If ((STA == One))
			{
				/* Disable IMG_CLK */
				Sleep(1) /* t0+t1 */
				MCOF(3) /* Clock 3 */

				/* Pull RST low */
#if CONFIG(BOARD_GOOGLE_VOLTEER)
				CTXS(GPP_F15)
#else
				CTXS(GPP_D4)
#endif

				/* Pull PWREN low */
				CTXS(GPP_H20)

				/* Pull SNRPWR_EN low */
				CTXS(GPP_H14)

				Store(0,STA)
			}
		}
		Method (_STA, 0, NotSerialized)  /* _STA: Status */
		{
			Return (STA)
		}
	}

	Device (CAM0)
	{
		Name (_HID, "OVTI8856")  /* _HID: Hardware ID */
		Name (_UID, Zero)  /* _UID: Unique ID */
		Name (_DDN, "Ov 8856 Camera")  /* _DDN: DOS Device Name */
		Method (_STA, 0, NotSerialized)  /* _STA: Status */
		{
			Return (0x0F)
		}
		Name (_CRS, ResourceTemplate ()  /* _CRS: Current Resource Settings */
		{
			I2cSerialBus (0x0010, ControllerInitiated, 0x00061A80,
				AddressingMode7Bit, "\\_SB.PCI0.I2C3",
				0x00, ResourceConsumer, ,
			)
		})
		Name (_PR0, Package (0x01)  /* _PR0: Power Resources for D0 */
		{
			RCPR
		})
		Name (_PR3, Package (0x01)  /* _PR3: Power Resources for D3hot */
		{
			RCPR
		})
		Name (_DSD, Package (0x04)  /* _DSD: Device-Specific Data */
		{
			ToUUID ("dbb8e3e6-5886-4ba6-8795-1319f52a966b"),
			Package (0x01)
			{
				Package (0x02)
				{
					"port0",
					"PRT0"
				}
			},
			ToUUID ("daffd814-6eba-4d8c-8a91-bc9bbf4aa301"),
			Package (0x03)
			{
				Package (0x02)
				{
					"clock-frequency",
					0x0124F800
				},
				Package (0x02)
				{
					"lens-focus",
					Package (0x01)
					{
						VCM0
					}
				},
				Package (0x02)
				{
					"i2c-allow-low-power-probe",
					0x01
 				}
			}
		})
		Name (PRT0, Package (0x04)
		{
			ToUUID ("daffd814-6eba-4d8c-8a91-bc9bbf4aa301"),
			Package (0x01)
			{
				Package (0x02)
				{
					"port",
					Zero
				}
			},
			ToUUID ("dbb8e3e6-5886-4ba6-8795-1319f52a966b"),
			Package (0x01)
			{
				Package (0x02)
				{
					"endpoint0",
					"EP00"
				}
			}
		})
		Name (EP00, Package (0x02)
		{
			ToUUID ("daffd814-6eba-4d8c-8a91-bc9bbf4aa301"),
			Package (0x05)
			{
				Package (0x02)
				{
					"endpoint",
					Zero
				},
				Package (0x02)
				{
					"clock-lanes",
					Zero
				},
				Package (0x02)
				{
					"data-lanes",
					Package (0x04)
					{
						One,
						0x02,
						0x03,
						0x04
					}
				},
				Package (0x02)
				{
					"link-frequencies",
					Package (0x02)
					{
						0x15752A00,
						0xABA9500
					}
				},
				Package (0x02)
				{
					"remote-endpoint",
					Package (0x03)
					{
						IPU0,
						Zero,
						Zero
					}
				}
			}
		})
	}

	Device (VCM0)
	{
		Name (_HID, "PRP0001")  /* _HID: Hardware ID */
		Name (_UID, 0x00)  /* _UID: Unique ID */
		Name (_DDN, "DW9768 VCM")  /* _DDN: DOS Device Name */
		Method (_STA, 0, NotSerialized)  /* _STA: Status */
		{
			Return (0x0F)
		}
		Name (_CRS, ResourceTemplate ()  /* _CRS: Current Resource Settings */
		{
			I2cSerialBus (0x000C, ControllerInitiated, 0x00061A80,
				AddressingMode7Bit, "\\_SB.PCI0.I2C3",
				0x00, ResourceConsumer, ,
			)
		})
		Name (_DEP, Package (0x01)  /* _DEP: Dependencies */
		{
			CAM0
		})
		Name (_PR0, Package (0x01)  /* _PR0: Power Resources for D0 */
		{
			RCPR
		})
		Name (_PR3, Package (0x01)  /* _PR3: Power Resources for D3hot */
		{
			RCPR
		})
		Name (_DSD, Package (0x02)  /* _DSD: Device-Specific Data */
		{
			ToUUID ("daffd814-6eba-4d8c-8a91-bc9bbf4aa301"),
			Package (0x02)
			{
				Package (0x02)
				{
					"compatible",
					"dongwoon,dw9768"
				},
				Package (0x02)
				{
					"i2c-allow-low-power-probe",
					0x01
 				}
			}
		})
	}
	Device (NVM0)
	{
		Name (_HID, "PRP0001")  // _HID: Hardware ID
		Name (_UID, 0x01)  // _UID: Unique ID
		Name (_DDN, "AT24 EEPROM")  // _DDN: DOS Device Name
		Method (_STA, 0, NotSerialized)  // _STA: Status
		{
			Return (0x0F)
		}
		Name (_CRS, ResourceTemplate ()  // _CRS: Current Resource Settings
		{
			I2cSerialBusV2 (0x0058, ControllerInitiated, 0x00061A80,
				AddressingMode7Bit, "\\_SB.PCI0.I2C3",
				0x00, ResourceConsumer, , Exclusive,
				)
		})
		Name (_DEP, Package (0x01)  // _DEP: Dependencies
		{
			CAM0
		})
		Name (_PR0, Package (0x01)  // _PR0: Power Resources for D0
		{
			RCPR
		})
		Name (_PR3, Package (0x01)  // _PR3: Power Resources for D3hot
		{
			RCPR
		})
		Name (_DSD, Package (0x02)  // _DSD: Device-Specific Data
		{
			ToUUID ("daffd814-6eba-4d8c-8a91-bc9bbf4aa301") /* Device Properties for _DSD */,
			Package (0x06)
			{
				Package (0x02)
				{
					"size",
					0x2800
				},
				Package (0x02)
				{
					"pagesize",
					One
				},
				Package (0x02)
				{
					"read-only",
					One
				},
				Package (0x02)
				{
					"address-width",
					0x0D
				},
				Package (0x02)
				{
					"compatible",
					"atmel,24c1024"
				},
				Package (0x02)
				{
					"i2c-allow-low-power-probe",
					0x01
				}
			}
		})
	}
}

Scope (\_SB.PCI0.I2C2)
{
	PowerResource (FCPR, 0x00, 0x0000)
	{
		Name (STA, Zero)
		Method (_ON, 0, Serialized)  /* Front camera_ON_: Power On */
		{
			If ((STA == Zero))
			{
				/* Enable IMG_CLK */
				MCON(2,1) /* Clock 2, 19.2MHz */

				/* Pull RST low */
				CTXS(GPP_D4)

				/* Pull SNRPWR_EN high */
				STXS(GPP_D18)

				/* Pull PWREN high */
				STXS(GPP_D17)
				Sleep(10) /* t9 */

				/* Pull RST high */
				STXS(GPP_D4)
				Sleep(1) /* t2 */

				Store(1,STA)
			}
		}
		Method (_OFF, 0, Serialized)  /* Front camera_OFF_: Power Off */
		{
			If ((STA == One))
			{
				/* Disable IMG_CLK */
				Sleep(1) /* t0+t1 */
				MCOF(2) /* Clock 2 */

				/* Pull RST low */
				CTXS(GPP_D4)

				/* Pull PWREN low */
				CTXS(GPP_D17)

				/* Pull SNRPWR_EN low */
				CTXS(GPP_D18)

				Store(0,STA)
			}
		}
		Method (_STA, 0, NotSerialized)  /* _STA: Status */
		{
			Return (STA)
		}
	}

	Device (CAM1)
	{
		Name (_HID, "INT3474")  /* _HID: Hardware ID */
		Name (_UID, Zero)  /* _UID: Unique ID */
		Name (_DDN, "Ov 2740 Camera")  /* _DDN: DOS Device Name */
		Method (_STA, 0, NotSerialized)  /* _STA: Status */
		{
			Return (0x0F)
		}
		Name (_CRS, ResourceTemplate ()  /* _CRS: Current Resource Settings */
		{
			I2cSerialBus (0x0010, ControllerInitiated, 0x00061A80,
				AddressingMode7Bit, "\\_SB.PCI0.I2C2",
				0x00, ResourceConsumer, ,
			)
		})
		Name (_PR0, Package (0x01)  /* _PR0: Power Resources for D0 */
		{
			FCPR
		})
		Name (_PR3, Package (0x01)  /* _PR3: Power Resources for D3hot */
		{
			FCPR
		})
		Name (_DSD, Package (0x04)  /* _DSD: Device-Specific Data */
		{
			ToUUID ("dbb8e3e6-5886-4ba6-8795-1319f52a966b"),
			Package (0x01)
			{
				Package (0x02)
				{
					"port0",
					"PRT0"
				}
			},
			ToUUID ("daffd814-6eba-4d8c-8a91-bc9bbf4aa301"),
			Package (0x02)
			{
				Package (0x02)
				{
					"clock-frequency",
					0x0124F800
				},
				Package (0x02)
				{
					"i2c-allow-low-power-probe",
					0x01
 				}
			}
		})
		Name (PRT0, Package (0x04)
		{
			ToUUID ("daffd814-6eba-4d8c-8a91-bc9bbf4aa301"),
			Package (0x01)
			{
				Package (0x02)
				{
					"port",
					Zero
				}
			},
			ToUUID ("dbb8e3e6-5886-4ba6-8795-1319f52a966b"),
			Package (0x01)
			{
				Package (0x02)
				{
					"endpoint0",
					"EP00"
				}
			}
		})
		Name (EP00, Package (0x02)
		{
			ToUUID ("daffd814-6eba-4d8c-8a91-bc9bbf4aa301"),
			Package (0x05)
			{
				Package (0x02)
				{
					"endpoint",
					Zero
				},
				Package (0x02)
				{
					"clock-lanes",
					Zero
				},
				Package (0x02)
				{
					"data-lanes",
					Package (0x02)
					{
						One,
						0x02
					}
				},
				Package (0x02)
				{
					"link-frequencies",
					Package (0x01)
					{
						0x15752A00
					}
				},
				Package (0x02)
				{
					"remote-endpoint",
					Package (0x03)
					{
						IPU0,
						One,
						Zero
					}
				}
			}
		})
	}
}
