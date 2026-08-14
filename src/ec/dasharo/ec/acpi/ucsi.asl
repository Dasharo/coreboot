/* SPDX-License-Identifier: GPL-2.0-only */

Scope (\_SB)
{
	/*
	 * UCSI ACPI interface for Dasharo EC USB-PD (TPS65987).
	 *
	 * Transport: SMFI CMD_UCSI (0x1B) proxies UCSI CONTROL bytes to the TPS65987
	 * via its 4CC 'UCSI' command interface. The TPS65987 DataX response is mapped
	 * to UCSI CCI + MESSAGE_IN in the shared buffer.
	 *
	 * The UCSI shared buffer (OperationRegion UCSM, _CRS, and all field
	 * declarations) is generated at runtime by dasharo_ec_fill_ssdt() in
	 * dasharo_ec.c, which allocates a CBMEM region and emits the SSDT scope
	 * "\\_SB.UCSI" with the actual physical address. The External() declarations
	 * below reference those dynamically-generated fields.
	 *
	 * UCSI v1.0 data structure layout (48 bytes, per CBMEM allocation):
	 *   Offset  0 ( 2B): VERSION    = 0x0100
	 *   Offset  2 ( 2B): RESERVED
	 *   Offset  4 ( 4B): CCI        (PPM writes, OPM reads)
	 *   Offset  8 ( 8B): CONTROL    (OPM writes, PPM reads)
	 *   Offset 16 (16B): MESSAGE_IN (PPM writes, OPM reads)
	 *   Offset 32 (16B): MESSAGE_OUT (OPM writes, PPM reads -- future use)
	 *
	 * SMFI CMD_UCSI data layout (matches cmd_ucsi() in smfi.c):
	 *   Input  data[0..7]  = UCSI CONTROL (8 bytes)
	 *   Output data[0]     = response_len (N bytes in DataX)
	 *   Output data[1]     = TPS65987 task return code (0 = success)
	 *   Output data[2..N]  = TPS65987 DataX[2..N] -> UCSI MESSAGE_IN[0..]
	 *
	 * _DSM UUID: 6f8398c2-7ca4-11e4-ad36-631042b5008f (UCSI ACPI transport)
	 *   Function 0: return supported bitmask (0x07 = functions 0, 1, 2)
	 *   Function 1: OS has written CONTROL -> EC executes, populates CCI+MESSAGE_IN, notifies
	 *   Function 2: OS acknowledges CCI read (no-op; data already in shared buffer)
	 */

	/* Byte-level access to SMFI DATA region for UCSI command exchange.
	 * SMFI OperationRegion is defined in smfi.asl; additional Field overlays
	 * on the same region are permitted by the AML spec. */
	Field (SMFI, ByteAcc, Lock, Preserve)
	{
		Offset (0x02),	/* Skip CMD (0x00) and RESULT (0x01) */
		SD00, 8,	/* data[0]:  CONTROL[0] in / response_len out */
		SD01, 8,	/* data[1]:  CONTROL[1] in / task return code out */
		SD02, 8,	/* data[2]:  CONTROL[2] in / MESSAGE_IN[0] out */
		SD03, 8,	/* data[3]:  CONTROL[3] in / MESSAGE_IN[1] out */
		SD04, 8,	/* data[4]:  CONTROL[4] in / MESSAGE_IN[2] out */
		SD05, 8,	/* data[5]:  CONTROL[5] in / MESSAGE_IN[3] out */
		SD06, 8,	/* data[6]:  CONTROL[6] in / MESSAGE_IN[4] out */
		SD07, 8,	/* data[7]:  CONTROL[7] in / MESSAGE_IN[5] out */
		SD08, 8,	/* data[8]:  MESSAGE_IN[6] out */
		SD09, 8,	/* data[9]:  MESSAGE_IN[7] out */
		SD0A, 8,	/* data[10]: MESSAGE_IN[8] out */
		SD0B, 8,	/* data[11]: MESSAGE_IN[9] out */
		SD0C, 8,	/* data[12]: MESSAGE_IN[10] out */
		SD0D, 8,	/* data[13]: MESSAGE_IN[11] out */
		SD0E, 8,	/* data[14]: MESSAGE_IN[12] out */
		SD0F, 8,	/* data[15]: MESSAGE_IN[13] out */
		SD10, 8,	/* data[16]: MESSAGE_IN[14] out */
	}

	Device (UCSI)
	{
		Name (_HID, EisaId ("PNP0CA0"))
		Name (_DDN, "Dasharo EC USB-C UCSI")
		Name (_UID, 1)
		Name (_STA, 0xF)

		/*
		 * _CRS, OperationRegion (UCSM), and field declarations are generated
		 * at runtime by dasharo_ec_fill_ssdt() into the SSDT under this scope.
		 */
		External (VER0, FieldUnitObj)
		External (VER1, FieldUnitObj)
		External (CCI0, FieldUnitObj)
		External (CCI1, FieldUnitObj)
		External (CCI2, FieldUnitObj)
		External (CCI3, FieldUnitObj)
		External (CTL0, FieldUnitObj)
		External (CTL1, FieldUnitObj)
		External (CTL2, FieldUnitObj)
		External (CTL3, FieldUnitObj)
		External (CTL4, FieldUnitObj)
		External (CTL5, FieldUnitObj)
		External (CTL6, FieldUnitObj)
		External (CTL7, FieldUnitObj)
		External (MGI0, FieldUnitObj)
		External (MGI1, FieldUnitObj)
		External (MGI2, FieldUnitObj)
		External (MGI3, FieldUnitObj)
		External (MGI4, FieldUnitObj)
		External (MGI5, FieldUnitObj)
		External (MGI6, FieldUnitObj)
		External (MGI7, FieldUnitObj)
		External (MGI8, FieldUnitObj)
		External (MGI9, FieldUnitObj)
		External (MGIA, FieldUnitObj)
		External (MGIB, FieldUnitObj)
		External (MGIC, FieldUnitObj)
		External (MGID, FieldUnitObj)
		External (MGIE, FieldUnitObj)
		External (MGIF, FieldUnitObj)
		External (MGO0, FieldUnitObj)
		External (MGO1, FieldUnitObj)
		External (MGO2, FieldUnitObj)
		External (MGO3, FieldUnitObj)
		External (MGO4, FieldUnitObj)
		External (MGO5, FieldUnitObj)
		External (MGO6, FieldUnitObj)
		External (MGO7, FieldUnitObj)
		External (MGO8, FieldUnitObj)
		External (MGO9, FieldUnitObj)
		External (MGOA, FieldUnitObj)
		External (MGOB, FieldUnitObj)
		External (MGOC, FieldUnitObj)
		External (MGOD, FieldUnitObj)
		External (MGOE, FieldUnitObj)
		External (MGOF, FieldUnitObj)

		/* Write UCSI version into shared buffer on device init */
		Method (_INI, 0, Serialized)
		{
			VER0 = 0x00
			VER1 = 0x01	/* Version 1.0 (little-endian 0x0100) */
		}

		/*
		 * DSND: Execute a UCSI command via SMFI CMD_UCSI.
		 *
		 * Reads CONTROL from the shared buffer (CTL0..CTL7), writes it byte-by-byte
		 * to the SMFI DATA region (bypassing SMFC which would wipe all DATA bytes),
		 * issues CMD_UCSI = 0x1B, then maps the TPS65987 DataX response to
		 * UCSI CCI + MESSAGE_IN in the shared buffer.
		 *
		 * CCI byte mapping:
		 *   CCI[2] bit 7 = CommandCompleted, bit 6 = Error, bit 5 = ResetCompleted
		 *   CCI[1] = DataLength = number of valid MESSAGE_IN bytes
		 *          = response_len - 1 (subtract the task return code byte)
		 */
		Method (DSND, 0, Serialized)
		{
			Local0 = 10	/* Pre-command timeout counter (ms) */

			/* Wait for any previous SMFI command to finish */
			While (SMCR != 0)
			{
				Sleep (1)
				Local0--
				If (Local0 == 0)
				{
					CCI0 = 0
					CCI1 = 0
					CCI2 = 0x40	/* ErrorIndicator = bit 22 of CCI = CCI2[6] */
					CCI3 = 0
					Notify (\_SB.UCSI, 0x80)
					Return
				}
			}

			/* Write UCSI CONTROL bytes to SMFI DATA[0..7] */
			SD00 = CTL0
			SD01 = CTL1
			SD02 = CTL2
			SD03 = CTL3
			SD04 = CTL4
			SD05 = CTL5
			SD06 = CTL6
			SD07 = CTL7

			/* Issue CMD_UCSI (27 = 0x1B) */
			SMCR = 0x1B

			/* Poll for command completion (~200 ms timeout) */
			Local0 = 200
			While (SMCR != 0)
			{
				Sleep (1)
				Local0--
				If (Local0 == 0)
				{
					CCI0 = 0
					CCI1 = 0
					CCI2 = 0x40	/* ErrorIndicator = bit 22 of CCI = CCI2[6] */
					CCI3 = 0
					Notify (\_SB.UCSI, 0x80)
					Return
				}
			}

			/*
			 * Map TPS65987 response to UCSI CCI + MESSAGE_IN.
			 * SMRR = SMFI result (0 = RES_OK), SD01 = TPS65987 task return code.
			 */
			If (LAnd (SMRR == 0, SD01 == 0))
			{
				/* Success */
				Local1 = SD00		/* response_len */
				If (Local1 > 0) { Local1-- }	/* subtract task return code byte */

				CCI0 = 0
				CCI1 = Local1		/* DataLength: valid MESSAGE_IN bytes */
				/* CommandCompleted = bit 23 of CCI = CCI2[7] = 0x80 */
				/* ResetCompleted   = bit 21 of CCI = CCI2[5] = 0x20 */
				If (CTL0 == 0x01)	/* PPM Reset command */
				{
					CCI2 = 0xA0	/* CommandCompleted | ResetCompleted */
				}
				Else
				{
					CCI2 = 0x80	/* CommandCompleted */
				}
				CCI3 = 0

				MGI0 = SD02
				MGI1 = SD03
				MGI2 = SD04
				MGI3 = SD05
				MGI4 = SD06
				MGI5 = SD07
				MGI6 = SD08
				MGI7 = SD09
				MGI8 = SD0A
				MGI9 = SD0B
				MGIA = SD0C
				MGIB = SD0D
				MGIC = SD0E
				MGID = SD0F
				MGIE = SD10
				MGIF = 0
			}
			Else
			{
				/* Error: SMFI failed or TPS65987 returned non-zero task code */
				CCI0 = 0
				CCI1 = 0
				CCI2 = 0x40		/* ErrorIndicator = bit 22 of CCI = CCI2[6] */
				CCI3 = 0

				MGI0 = 0
				MGI1 = 0
				MGI2 = 0
				MGI3 = 0
				MGI4 = 0
				MGI5 = 0
				MGI6 = 0
				MGI7 = 0
				MGI8 = 0
				MGI9 = 0
				MGIA = 0
				MGIB = 0
				MGIC = 0
				MGID = 0
				MGIE = 0
				MGIF = 0
			}

			/* Signal OS that PPM has completed the command */
			Notify (\_SB.UCSI, 0x80)
		}

		Method (_DSM, 4, Serialized)
		{
			If (Arg0 != ToUUID ("6f8398c2-7ca4-11e4-ad36-631042b5008f")) {
				Return (Buffer (1) { 0 })
			}

			Switch (ToInteger (Arg2))
			{
				Case (0)
				{
					/* Query supported functions: 0, 1, 2 */
					Return (Buffer (1) { 0x07 })
				}
				Case (1)
				{
					/*
					 * Send UCSI Data to PPM.
					 * OS driver has written CONTROL to CTL0..CTL7 in the
					 * shared buffer. Execute via SMFI and update CCI + MESSAGE_IN.
					 */
					DSND ()
				}
				Case (2)
				{
					/*
					 * Get UCSI Data from PPM (OPM acknowledge).
					 * CCI and MESSAGE_IN are already in the shared buffer
					 * from the completed DSND() call. Nothing to do.
					 */
				}
			}

			Return (Buffer (1) { 0 })
		}
	}
}