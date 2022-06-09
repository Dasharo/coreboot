/* SPDX-License-Identifier: GPL-2.0-only */

#ifndef __SR5650_H__
#define __SR5650_H__

#include <stdint.h>
#include <acpi/acpi.h>
#include "chip.h"
#include "rev.h"

typedef struct __PCIE_CFG__ {
	u16 Config;
	u8 ResetReleaseDelay;
	u8 Gfx0Width;
	u8 Gfx1Width;
	u8 GfxPayload;
	u8 GppPayload;
	u16 PortDetect;
	u8 PortHp;		/* hot plug */
	u16 DbgConfig;
	u32 DbgConfig2;
	u8 GfxLx;
	u8 GppLx;
	u8 NBSBLx;
	u8 PortSlotInit;
	u8 Gfx0Pwr;
	u8 Gfx1Pwr;
	u8 GppPwr;
} PCIE_CFG;

/* PCIE config flags */
#define	PCIE_DUALSLOT_CONFIG		(1 << 0)
#define	PCIE_OVERCLOCK_ENABLE		(1 << 1)
#define	PCIE_GPP_CLK_GATING		(1 << 2)
#define	PCIE_ENABLE_STATIC_DEV_REMAP	(1 << 3)
#define	PCIE_OFF_UNUSED_GFX_LANES	(1 << 4)
#define	PCIE_OFF_UNUSED_GPP_LANES	(1 << 5)
#define	PCIE_DISABLE_HIDE_UNUSED_PORTS	(1 << 7)
#define	PCIE_GFX_CLK_GATING		(1 << 11)
#define	PCIE_GFX_COMPLIANCE		(1 << 14)
#define	PCIE_GPP_COMPLIANCE		(1 << 15)

/* -------------------- ----------------------
* NBMISCIND
 ------------------- -----------------------*/
#define	PCIE_LINK_CFG			0x8
#define	PCIE_NBCFG_REG7			0x37
#define	STRAPS_OUTPUT_MUX_7		0x67
#define	STRAPS_OUTPUT_MUX_A		0x6a

/* -------------------- ----------------------
* PCIEIND
 ------------------- -----------------------*/
#define	PCIE_CI_CNTL			0x20
#define	PCIE_LC_LINK_WIDTH		0xa2
#define PCIE_LC_STATE0			0xa5
#define	PCIE_VC0_RESOURCE_STATUS	0x12a	/* 16bit read only */

#define	PCIE_CORE_INDEX_SB		(0x05 << 16) /* see rpr 4.3.2.2, bdg 2.1 */
#define	PCIE_CORE_INDEX_GPP1		(0x04 << 16)
#define	PCIE_CORE_INDEX_GPP2		(0x06 << 16)
#define	PCIE_CORE_INDEX_GPP1_GPP2	(0x00 << 16)
#define	PCIE_CORE_INDEX_GPP3a		(0x07 << 16)
#define	PCIE_CORE_INDEX_GPP3b		(0x03 << 16)

/* contents of PCIE_VC0_RESOURCE_STATUS */
#define	VC_NEGOTIATION_PENDING		(1 << 1)

#define	LC_STATE_RECONFIG_GPPSB		0x10

#define IO_APIC2_ADDR			0xfec20000
/* ------------------------------------------------
* Global variable
* ------------------------------------------------- */
extern PCIE_CFG AtiPcieCfg;

void enable_pcie_bar3(void);
void sr5650_disable_pcie_bridge(void);
void enable_sr5650_dev8(void);

u8 PcieTrainPort(struct device *nb_dev, struct device *dev, u32 port);
void PcieReleasePortTraining(struct device *nb_dev, struct device *dev, u32 port);

u32 nbpcie_p_read_index(struct device *dev, u32 index);
void nbpcie_p_write_index(struct device *dev, u32 index, u32 data);
u32 nbpcie_ind_read_index(struct device *nb_dev, u32 index);
void nbpcie_ind_write_index(struct device *nb_dev, u32 index, u32 data);

void init_gen2(struct device *nb_dev, struct device *dev, u8 port);
void sr5650_gpp_sb_init(struct device *nb_dev, struct device *dev, u32 port);
void sr56x0_lock_hwinitreg(void);
void config_gpp_core(struct device *nb_dev, struct device *sb_dev);
void pcie_config_misc_clk(struct device *nb_dev);

#endif
