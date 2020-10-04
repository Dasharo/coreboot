/* SPDX-License-Identifier: GPL-2.0-only */

/* The _WAK method is called on system wakeup */

Method(_WAK,1)
{
	// CPU specific part

	// Notify PCI Express slots in case a card
	// was inserted while a sleep state was active.

	// Are we going to S3?
	If (Arg0 == 3) {
		// ..
	}

	// Are we going to S4?
	If (Arg0 == 4) {
		// ..
	}

	// TODO: Windows XP SP2 P-State restore

	Return(Package(){0,0})
}

/* System Bus */

Scope(\_SB)
{
	/* This method is placed on the top level, so we can make sure it's the
	 * first executed _INI method.
	 */
	Method(_INI, 0)
	{
		/* The DTS data in NVS is probably not up to date.
		 * Update temperature values and make sure AP thermal
		 * interrupts can happen
		 */

		// TRAP(71) // TODO

		\GOS()

		/* And the OS workarounds start right after we know what we're
		 * running: Windows XP SP1 needs to have C-State coordination
		 * enabled in SMM.
		 */
		If ((OSYS == 2001) && MPEN) {
			// TRAP(61) // TODO
		}

		/* SMM power state and C4-on-C3 settings need to be updated */
		// TRAP(43) // TODO
	}
}
