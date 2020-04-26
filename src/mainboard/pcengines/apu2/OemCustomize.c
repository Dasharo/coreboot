/* SPDX-License-Identifier: GPL-2.0-only */

#include <AGESA.h>
#include <dasharo/options.h>
#include <northbridge/amd/agesa/state_machine.h>

#include "gpio_ftns.h"

/*
 * The configuration data is duplicated for different states of power management
 * because modifiable globals aren't allowed in romstage.
 */

static const PCIe_PORT_DESCRIPTOR PortListAspm[] = {
	{
		0,
		PCIE_ENGINE_DATA_INITIALIZER(PciePortEngine, 3, 3),
		PCIE_PORT_DATA_INITIALIZER_V2(PortEnabled, ChannelTypeExt6db, 2, 5,
				HotplugDisabled,
				PcieGenMaxSupported,
				PcieGenMaxSupported,
				AspmL0sL1,
				0x01,
				1)
	},
	/* Initialize Port descriptor (PCIe port, Lanes 1, PCI Device Number 2, ...) */
	{
		0,
		PCIE_ENGINE_DATA_INITIALIZER(PciePortEngine, 2, 2),
		PCIE_PORT_DATA_INITIALIZER_V2(PortEnabled, ChannelTypeExt6db, 2, 4,
				HotplugDisabled,
				PcieGenMaxSupported,
				PcieGenMaxSupported,
				AspmL0sL1,
				0x02,
				1)
	},
	/* Initialize Port descriptor (PCIe port, Lanes 2, PCI Device Number 2, ...) */
	{
		0,
		PCIE_ENGINE_DATA_INITIALIZER(PciePortEngine, 1, 1),
		PCIE_PORT_DATA_INITIALIZER_V2(PortEnabled, ChannelTypeExt6db, 2, 3,
				HotplugDisabled,
				PcieGenMaxSupported,
				PcieGenMaxSupported,
				AspmL0sL1,
				0x03,
				1)
	},
	/* Initialize Port descriptor (PCIe port, Lanes 3, PCI Device Number 2, ...) */
	{
		0,
		PCIE_ENGINE_DATA_INITIALIZER(PciePortEngine, 0, 0),
		PCIE_PORT_DATA_INITIALIZER_V2(PortEnabled, ChannelTypeExt6db, 2, 2,
				HotplugDisabled,
				PcieGenMaxSupported,
				PcieGenMaxSupported,
				AspmL0sL1,
				0x04,
				1)
	},
	/* Initialize Port descriptor (PCIe port, Lanes 4-7, PCI Device Number 4, ...) */
	{
		DESCRIPTOR_TERMINATE_LIST,
		PCIE_ENGINE_DATA_INITIALIZER(PciePortEngine, 4, 7),
		PCIE_PORT_DATA_INITIALIZER_V2(PortEnabled, ChannelTypeExt6db, 2, 1,
				HotplugDisabled,
				PcieGenMaxSupported,
				PcieGenMaxSupported,
				AspmL0sL1,
				0x05,
				1)
	}
};

static const PCIe_PORT_DESCRIPTOR PortList[] = {
	{
		0,
		PCIE_ENGINE_DATA_INITIALIZER(PciePortEngine, 3, 3),
		PCIE_PORT_DATA_INITIALIZER_V2(PortEnabled, ChannelTypeExt6db, 2, 5,
				HotplugDisabled,
				PcieGenMaxSupported,
				PcieGenMaxSupported,
				AspmDisabled,
				0x01,
				0)
	},
	/* Initialize Port descriptor (PCIe port, Lanes 1, PCI Device Number 2, ...) */
	{
		0,
		PCIE_ENGINE_DATA_INITIALIZER(PciePortEngine, 2, 2),
		PCIE_PORT_DATA_INITIALIZER_V2(PortEnabled, ChannelTypeExt6db, 2, 4,
				HotplugDisabled,
				PcieGenMaxSupported,
				PcieGenMaxSupported,
				AspmDisabled,
				0x02,
				0)
	},
	/* Initialize Port descriptor (PCIe port, Lanes 2, PCI Device Number 2, ...) */
	{
		0,
		PCIE_ENGINE_DATA_INITIALIZER(PciePortEngine, 1, 1),
		PCIE_PORT_DATA_INITIALIZER_V2(PortEnabled, ChannelTypeExt6db, 2, 3,
				HotplugDisabled,
				PcieGenMaxSupported,
				PcieGenMaxSupported,
				AspmDisabled,
				0x03,
				0)
	},
	/* Initialize Port descriptor (PCIe port, Lanes 3, PCI Device Number 2, ...) */
	{
		0,
		PCIE_ENGINE_DATA_INITIALIZER(PciePortEngine, 0, 0),
		PCIE_PORT_DATA_INITIALIZER_V2(PortEnabled, ChannelTypeExt6db, 2, 2,
				HotplugDisabled,
				PcieGenMaxSupported,
				PcieGenMaxSupported,
				AspmDisabled,
				0x04,
				0)
	},
	/* Initialize Port descriptor (PCIe port, Lanes 4-7, PCI Device Number 4, ...) */
	{
		DESCRIPTOR_TERMINATE_LIST,
		PCIE_ENGINE_DATA_INITIALIZER(PciePortEngine, 4, 7),
		PCIE_PORT_DATA_INITIALIZER_V2(PortEnabled, ChannelTypeExt6db, 2, 1,
				HotplugDisabled,
				PcieGenMaxSupported,
				PcieGenMaxSupported,
				AspmDisabled,
				0x05,
				0)
	}
};

static const PCIe_COMPLEX_DESCRIPTOR PcieComplex = {
	.Flags        = DESCRIPTOR_TERMINATE_LIST,
	.SocketId     = 0,
	.PciePortList = PortList,
	.DdiLinkList  = NULL,
};

static const PCIe_COMPLEX_DESCRIPTOR PcieComplexAspm = {
	.Flags        = DESCRIPTOR_TERMINATE_LIST,
	.SocketId     = 0,
	.PciePortList = PortListAspm,
	.DdiLinkList  = NULL,
};

void board_BeforeInitEarly(struct sysinfo *cb, AMD_EARLY_PARAMS *InitEarly)
{
	if (dasharo_apu_pcie_pm_enabled())
		InitEarly->GnbConfig.PcieComplexList = &PcieComplexAspm;
	else
		InitEarly->GnbConfig.PcieComplexList = &PcieComplex;
	InitEarly->PlatformConfig.CStateMode = CStateModeC6;
	InitEarly->PlatformConfig.CpbMode = CpbModeAuto;
}

void board_BeforeInitPost(struct sysinfo *cb, AMD_POST_PARAMS *Post)
{
	/*
	 * Bank interleaving does not work on this platform.
	 * Disable it so AGESA will return success.
	 */
	Post->MemConfig.EnableBankIntlv = FALSE;
	/* 4GB variants have ECC */
	if (get_spd_offset())
		Post->MemConfig.EnableEccFeature = TRUE;
	else
		Post->MemConfig.EnableEccFeature = FALSE;
}
