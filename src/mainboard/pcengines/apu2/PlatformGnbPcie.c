/*
 * This file is part of the coreboot project.
 *
 * Copyright (C) 2012 Advanced Micro Devices, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
 */

#include <northbridge/amd/pi/agesawrapper.h>
#include "bios_knobs.h"
#include <southbridge/amd/pi/hudson/hudson.h>
#include <console/console.h>
#include <console/loglevel.h>
#include <smp/node.h>

#define FILECODE PROC_GNB_PCIE_FAMILY_0X15_F15PCIECOMPLEXCONFIG_FILECODE

const PCIe_PORT_DESCRIPTOR PortList [] = {
	{
		0, //Descriptor flags  !!!IMPORTANT!!! Terminate last element of array
		PCIE_ENGINE_DATA_INITIALIZER (PciePortEngine, 3, 3),
		PCIE_PORT_DATA_INITIALIZER_V2 (PortEnabled, ChannelTypeExt6db, 2, 5,
				HotplugDisabled,
				PcieGenMaxSupported,
				PcieGenMaxSupported,
				AspmDisabled, 0x01, 0)
	},
	/* Initialize Port descriptor (PCIe port, Lanes 1, PCI Device Number 2, ...) */
	{
		0, //Descriptor flags  !!!IMPORTANT!!! Terminate last element of array
		PCIE_ENGINE_DATA_INITIALIZER (PciePortEngine, 2, 2),
		PCIE_PORT_DATA_INITIALIZER_V2 (PortEnabled, ChannelTypeExt6db, 2, 4,
				HotplugDisabled,
				PcieGenMaxSupported,
				PcieGenMaxSupported,
				AspmDisabled, 0x02, 0)
	},
	/* Initialize Port descriptor (PCIe port, Lanes 2, PCI Device Number 2, ...) */
	{
		0, //Descriptor flags  !!!IMPORTANT!!! Terminate last element of array
		PCIE_ENGINE_DATA_INITIALIZER (PciePortEngine, 1, 1),
		PCIE_PORT_DATA_INITIALIZER_V2 (PortEnabled, ChannelTypeExt6db, 2, 3,
				HotplugDisabled,
				PcieGenMaxSupported,
				PcieGenMaxSupported,
				AspmDisabled, 0x03, 0)
	},
	/* Initialize Port descriptor (PCIe port, Lanes 3, PCI Device Number 2, ...) */
	{
		0,
		PCIE_ENGINE_DATA_INITIALIZER (PciePortEngine, 0, 0),
		PCIE_PORT_DATA_INITIALIZER_V2 (PortEnabled, ChannelTypeExt6db, 2, 2,
				HotplugDisabled,
				PcieGenMaxSupported,
				PcieGenMaxSupported,
				AspmDisabled, 0x04, 0)
	},
	/* Initialize Port descriptor (PCIe port, Lanes 4-7, PCI Device Number 4, ...) */
	{
		DESCRIPTOR_TERMINATE_LIST, //Descriptor flags  !!!IMPORTANT!!! Terminate last element of array
		PCIE_ENGINE_DATA_INITIALIZER (PciePortEngine, 4, 7),
		PCIE_PORT_DATA_INITIALIZER_V2 (PortEnabled, ChannelTypeExt6db, 2, 1,
				HotplugDisabled,
				PcieGenMaxSupported,
				PcieGenMaxSupported,
				AspmDisabled, 0x05, 0)
	}
};

const PCIe_PORT_DESCRIPTOR PortListReverse [] = {
	{
		0, //Descriptor flags  !!!IMPORTANT!!! Terminate last element of array
		PCIE_ENGINE_DATA_INITIALIZER (PciePortEngine, 3, 3),
		PCIE_PORT_DATA_INITIALIZER_V2 (PortEnabled, ChannelTypeExt6db, 2, 4,
				HotplugDisabled,
				PcieGenMaxSupported,
				PcieGenMaxSupported,
				AspmDisabled, 0x01, 0)
	},
	/* Initialize Port descriptor (PCIe port, Lanes 1, PCI Device Number 2, ...) */
	{
		0, //Descriptor flags  !!!IMPORTANT!!! Terminate last element of array
		PCIE_ENGINE_DATA_INITIALIZER (PciePortEngine, 2, 2),
		PCIE_PORT_DATA_INITIALIZER_V2 (PortEnabled, ChannelTypeExt6db, 2, 3,
				HotplugDisabled,
				PcieGenMaxSupported,
				PcieGenMaxSupported,
				AspmDisabled, 0x02, 0)
	},
	/* Initialize Port descriptor (PCIe port, Lanes 2, PCI Device Number 2, ...) */
	{
		0, //Descriptor flags  !!!IMPORTANT!!! Terminate last element of array
		PCIE_ENGINE_DATA_INITIALIZER (PciePortEngine, 1, 1),
		PCIE_PORT_DATA_INITIALIZER_V2 (PortEnabled, ChannelTypeExt6db, 2, 2,
				HotplugDisabled,
				PcieGenMaxSupported,
				PcieGenMaxSupported,
				AspmDisabled, 0x03, 0)
	},
	/* Initialize Port descriptor (PCIe port, Lanes 3, PCI Device Number 2, ...) */
	{
		0,
		PCIE_ENGINE_DATA_INITIALIZER (PciePortEngine, 0, 0),
		PCIE_PORT_DATA_INITIALIZER_V2 (PortEnabled, ChannelTypeExt6db, 2, 1,
				HotplugDisabled,
				PcieGenMaxSupported,
				PcieGenMaxSupported,
				AspmDisabled, 0x04, 0)
	},
	/* Initialize Port descriptor (PCIe port, Lanes 4-7, PCI Device Number 4, ...) */
	{
		DESCRIPTOR_TERMINATE_LIST, //Descriptor flags  !!!IMPORTANT!!! Terminate last element of array
		PCIE_ENGINE_DATA_INITIALIZER (PciePortEngine, 4, 7),
		PCIE_PORT_DATA_INITIALIZER_V2 (PortEnabled, ChannelTypeExt6db, 2, 5,
				HotplugDisabled,
				PcieGenMaxSupported,
				PcieGenMaxSupported,
				AspmDisabled, 0x05, 0)
	}
};

const PCIe_DDI_DESCRIPTOR DdiList [] = {
	/* DP0 to HDMI0/DP */
	{
		0,
		PCIE_ENGINE_DATA_INITIALIZER (PcieDdiEngine, 8, 11),
		PCIE_DDI_DATA_INITIALIZER (ConnectorTypeDP, Aux1, Hdp1)
	},
	/* DP1 to FCH */
	{
		0,
		PCIE_ENGINE_DATA_INITIALIZER (PcieDdiEngine, 12, 15),
		PCIE_DDI_DATA_INITIALIZER (ConnectorTypeDP, Aux2, Hdp2)
	},
	/* DP2 to HDMI1/DP */
	{
		DESCRIPTOR_TERMINATE_LIST,
		PCIE_ENGINE_DATA_INITIALIZER (PcieDdiEngine, 16, 19),
		PCIE_DDI_DATA_INITIALIZER (ConnectorTypeCrt, Aux3, Hdp3)
	},
};

const PCIe_COMPLEX_DESCRIPTOR PcieComplex = {
	.Flags        = DESCRIPTOR_TERMINATE_LIST,
	.SocketId     = 0,
	.PciePortList = PortList,
	.DdiLinkList  = DdiList
};

const PCIe_COMPLEX_DESCRIPTOR PcieComplexReverse = {
	.Flags        = DESCRIPTOR_TERMINATE_LIST,
	.SocketId     = 0,
	.PciePortList = PortListReverse,
	.DdiLinkList  = DdiList
};

/*---------------------------------------------------------------------------------------*/
/**
 *  OemCustomizeInitEarly
 *
 *  Description:
 *    This stub function will call the host environment through the binary block
 *    interface (call-out port) to provide a user hook opportunity
 *
 *  Parameters:
 *    @param[in]      **PeiServices
 *    @param[in]      *InitEarly
 *
 *    @retval         VOID
 *
 **/
/*---------------------------------------------------------------------------------------*/
VOID
OemCustomizeInitEarly (
	IN  OUT AMD_EARLY_PARAMS    *InitEarly
	)
{
	if(check_pciereverse())
		InitEarly->GnbConfig.PcieComplexList = &PcieComplexReverse;
	else
		InitEarly->GnbConfig.PcieComplexList = &PcieComplex;

	if(check_boost()) {
		InitEarly->PlatformConfig.CStateMode = CStateModeC6;
		InitEarly->PlatformConfig.CpbMode = CpbModeAuto;
	}

	/* Enable or disable watchdog depending on the configuration*/
	if (boot_cpu()){
		volatile u32 *ptr = (u32 *)(ACPI_MMIO_BASE + WATCHDOG_BASE);
		u16 watchdog_timeout = get_watchdog_timeout();

		if (watchdog_timeout == 0) {
			//disable watchdog
			printk(BIOS_WARNING, "Watchdog is disabled\n");
			*ptr |= (1 << 3);
		} else {
			// enable
			*ptr &= ~(1 << 3);
			*ptr |= (1 << 0);
			// configure timeout
			*(ptr + 1) = (u16) watchdog_timeout;
			// trigger
			*ptr |= (1 << 7);
			printk(BIOS_WARNING, "Watchdog is enabled, state = 0x%x, time = %d\n", *ptr, *(ptr + 1));
		}
	}
}
