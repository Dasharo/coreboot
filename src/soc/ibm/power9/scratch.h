/* SPDX-License-Identifier: GPL-2.0-only */

#ifndef __SOC_IBM_POWER9_SCRATCH_H
#define __SOC_IBM_POWER9_SCRATCH_H

/*
 * This file is for common definitions related to
 * TP.TPVSB.FSI.W.FSI_MAILBOX.FSXCOMP.FSXLOG.SCRATCH_REGISTER_1
 * and the consecutive 7 scratch registers.
 */

/* SCOM address of the first scratch register */
#define MBOX_SCRATCH_REG1 0x00050038

/* CFAM address of the first scratch register (word addressing) */
#define MBOX_SCRATCH_REG1_FSI 0x00002838

#define MBOX_SCRATCH_REG6_GROUP_PUMP_MODE 23

#endif /* __SOC_IBM_POWER9_SCRATCH_H */
