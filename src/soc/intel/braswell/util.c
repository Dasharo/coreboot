/* SPDX-License-Identifier: GPL-2.0-only */

#include <device/pci_ops.h>

#include "chip.h"

/**
  Return SoC stepping type

  @retval SOC_STEPPING            SoC stepping type
**/
int SocStepping(void)
{
	const u8 revid = pci_read_config8(PCH_DEV_LPC, 0x8);

	switch (revid & B_PCH_LPC_RID_STEPPING_MASK) {
	case V_PCH_LPC_RID_A0:
		return SocA0;
	case V_PCH_LPC_RID_A1:
		return SocA1;
	case V_PCH_LPC_RID_A2:
		return SocA2;
	case V_PCH_LPC_RID_A3:
		return SocA3;
	case V_PCH_LPC_RID_A4:
		return SocA4;
	case V_PCH_LPC_RID_A5:
		return SocA5;
	case V_PCH_LPC_RID_A6:
		return SocA6;
	case V_PCH_LPC_RID_A7:
		return SocA7;
	case V_PCH_LPC_RID_B0:
		return SocB0;
	case V_PCH_LPC_RID_B1:
		return SocB1;
	case V_PCH_LPC_RID_B2:
		return SocB2;
	case V_PCH_LPC_RID_B3:
		return SocB3;
	case V_PCH_LPC_RID_B4:
		return SocB4;
	case V_PCH_LPC_RID_B5:
		return SocB5;
	case V_PCH_LPC_RID_B6:
		return SocB6;
	case V_PCH_LPC_RID_B7:
		return SocB7;
	case V_PCH_LPC_RID_C0:
		return SocC0;
	case V_PCH_LPC_RID_C1:
		return SocC1;
	case V_PCH_LPC_RID_C2:
		return SocC2;
	case V_PCH_LPC_RID_C3:
		return SocC3;
	case V_PCH_LPC_RID_C4:
		return SocC4;
	case V_PCH_LPC_RID_C5:
		return SocC5;
	case V_PCH_LPC_RID_C6:
		return SocC6;
	case V_PCH_LPC_RID_C7:
		return SocC7;
	case V_PCH_LPC_RID_D0:
		return SocD0;
	case V_PCH_LPC_RID_D1:
		return SocD1;
	case V_PCH_LPC_RID_D2:
		return SocD2;
	case V_PCH_LPC_RID_D3:
		return SocD3;
	case V_PCH_LPC_RID_D4:
		return SocD4;
	case V_PCH_LPC_RID_D5:
		return SocD5;
	case V_PCH_LPC_RID_D6:
		return SocD6;
	case V_PCH_LPC_RID_D7:
		return SocD7;
	default:
		return SocSteppingMax;
	}
}
