/* SPDX-License-Identifier: GPL-2.0-only */

#ifndef __PI_PICASSO_ACP_H__
#define __PI_PICASSO_ACP_H__

/* Bus A D0F5 - Audio Processor */
#define ACP_I2S_PIN_CONFIG	0x1400	/* HDA, Soundwire, I2S */
#define  PIN_CONFIG_MASK	(7 << 0)
#define ACP_I2S_WAKE_EN		0x1414
#define  WAKE_EN_MASK		(1 << 0)
#define ACP_PME_EN		0x1418
#define  PME_EN_MASK		(1 << 0)

#endif /* __PI_PICASSO_ACP_H__ */
