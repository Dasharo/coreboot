/* SPDX-License-Identifier: GPL-2.0-only */

#ifndef SOC_SMI_H
#define SOC_SMI_H

/* TODO: include Fch.h and use macros from there? */

#define SCI_GPES			32
#define SCIMAPS				64
#define SMI_GEVENTS			24
#define NUMBER_SMITYPES			160

#define SMITYPE_SMI_CMD_PORT		0x4b

#define SMI_EVENT_STATUS		0x00
#define SMI_SCI_TRIG			0x08
#define SMI_SCI_LEVEL			0x0c
#define SMI_SCI_STATUS			0x10
#define SMI_SCI_EN			0x14

#define SMI_SCI_MAP0			0x40
#define SMI_SCI_MAP(X)			(SMI_SCI_MAP0 + (X))

#define SMI_REG_SMISTS0			0x80
#define  GEVENT_MASK			 0x00ffffff
#define SMI_REG_SMISTS1			0x84
#define SMI_REG_SMISTS2			0x88
#define SMI_REG_SMISTS3			0x8c
#define SMI_REG_SMISTS4			0x90

#define SMI_REG_POINTER			0x94
#define  SMI_STATUS_SRC_SCI		 (1 << 0)
#define  SMI_STATUS_SRC_0		 (1 << 1)
#define  SMI_STATUS_SRC_1		 (1 << 2)
#define  SMI_STATUS_SRC_2		 (1 << 3)
#define  SMI_STATUS_SRC_3		 (1 << 4)
#define  SMI_STATUS_SRC_4		 (1 << 5)

#define SMI_REG_SMITRIG0		0x98
#define  SMITRG0_SMIENB			 (1 << 31)
#define  SMITRG0_EOS			 (1 << 28)

#define SMI_REG_CONTROL0		0xa0

#endif /* SOC_SMI_H */
