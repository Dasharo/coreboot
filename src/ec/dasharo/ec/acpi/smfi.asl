OperationRegion (SMFI, SystemIO, 0xE00, 0x100)
Field (SMFI, ByteAcc, Lock, Preserve)
{
	SMCR, 8,    // REG_CMD
	SMRR, 8,    // REG_RESULT
	SMDR, 2032, // REG_DATA
}

Method (SMFC, 2, Serialized) { // SMFI Command
	Local0 = 10 // Command completion timeout
	// Wait for previous command completion
	While (SMCR != 0)
	{
		Sleep (1)
		Local0--
		If (Local0 == 0)
		{
			Return (1)
		}
	}

	SMDR = Arg1
	SMCR = Arg0

	// Wait for command completion
	While (SMCR != 0)
	{
		Sleep (1)
		Local0--
		If (Local0 == 0)
		{
			Return (1)
		}
	}

	Return (SMRR)
}
