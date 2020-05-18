/* SPDX-License-Identifier: GPL-2.0-only */

#ifndef _MAINBOARD_HSIO_H
#define _MAINBOARD_HSIO_H

#include <fsp/util.h>

#ifndef __ACPI__
const BL_HSIO_INFORMATION harcuvar_hsio_config[] = {
	/*
	 * Supported Lanes:
	 *    20
	 *
	 * Bifurcation:
	 *    PCIE cluster #0: x8
	 *    PCIE cluster #1: x4x4
	 *
	 * FIA MUX config:
	 *    Lane[00:07]->x8 PCIE slot
	 *    Lane[08:11]->a x4 PCIe slot
	 *    Lane[12:15]->a 2nd x4 PCIe slot
	 *    Lane[16]->a SATA connector with pin7 to 5V adapter capable
	 *    Lane[17:18]  ->  2 SATA connectors
	 *    Lane[19]->USB3 rear I/O panel connector
	 */

	/* SKU HSIO 20 (pcie [0-15] sata [16-18] USB [19]) */
	{BL_SKU_HSIO_20,
	{PCIE_BIF_CTRL_x8, PCIE_BIF_CTRL_x4x4},
	{/* ME_FIA_MUX_CONFIG */
	  {BL_FIA_LANE_CONFIG(BL_ME_FIA_MUX_LANE_PCIE, BL_FIA_LANE00) |
	   BL_FIA_LANE_CONFIG(BL_ME_FIA_MUX_LANE_PCIE, BL_FIA_LANE01) |
	   BL_FIA_LANE_CONFIG(BL_ME_FIA_MUX_LANE_PCIE, BL_FIA_LANE02) |
	   BL_FIA_LANE_CONFIG(BL_ME_FIA_MUX_LANE_PCIE, BL_FIA_LANE03) |
	   BL_FIA_LANE_CONFIG(BL_ME_FIA_MUX_LANE_PCIE, BL_FIA_LANE04) |
	   BL_FIA_LANE_CONFIG(BL_ME_FIA_MUX_LANE_PCIE, BL_FIA_LANE05) |
	   BL_FIA_LANE_CONFIG(BL_ME_FIA_MUX_LANE_PCIE, BL_FIA_LANE06) |
	   BL_FIA_LANE_CONFIG(BL_ME_FIA_MUX_LANE_PCIE, BL_FIA_LANE07) |
	   BL_FIA_LANE_CONFIG(BL_ME_FIA_MUX_LANE_PCIE, BL_FIA_LANE08) |
	   BL_FIA_LANE_CONFIG(BL_ME_FIA_MUX_LANE_PCIE, BL_FIA_LANE09) |
	   BL_FIA_LANE_CONFIG(BL_ME_FIA_MUX_LANE_PCIE, BL_FIA_LANE10) |
	   BL_FIA_LANE_CONFIG(BL_ME_FIA_MUX_LANE_PCIE, BL_FIA_LANE11) |
	   BL_FIA_LANE_CONFIG(BL_ME_FIA_MUX_LANE_PCIE, BL_FIA_LANE12) |
	   BL_FIA_LANE_CONFIG(BL_ME_FIA_MUX_LANE_PCIE, BL_FIA_LANE13) |
	   BL_FIA_LANE_CONFIG(BL_ME_FIA_MUX_LANE_PCIE, BL_FIA_LANE14) |
	   BL_FIA_LANE_CONFIG(BL_ME_FIA_MUX_LANE_PCIE, BL_FIA_LANE15) |
	   BL_FIA_LANE_CONFIG(BL_ME_FIA_MUX_LANE_SATA, BL_FIA_LANE16) |
	   BL_FIA_LANE_CONFIG(BL_ME_FIA_MUX_LANE_SATA, BL_FIA_LANE17) |
	   BL_FIA_LANE_CONFIG(BL_ME_FIA_MUX_LANE_SATA, BL_FIA_LANE18) |
	   BL_FIA_LANE_CONFIG(BL_ME_FIA_MUX_LANE_XHCI, BL_FIA_LANE19)},

	  /* ME_FIA_SATA_CONFIG */
	  {BL_FIA_SATA_LANE_CONFIG(BL_ME_FIA_SATA_CONTROLLER_LANE_NOT_ASSIGNED,
				   BL_FIA_SATA_LANE04) |
	   BL_FIA_SATA_LANE_CONFIG(BL_ME_FIA_SATA_CONTROLLER_LANE_NOT_ASSIGNED,
				   BL_FIA_SATA_LANE05) |
	   BL_FIA_SATA_LANE_CONFIG(BL_ME_FIA_SATA_CONTROLLER_LANE_NOT_ASSIGNED,
				   BL_FIA_SATA_LANE06) |
	   BL_FIA_SATA_LANE_CONFIG(BL_ME_FIA_SATA_CONTROLLER_LANE_NOT_ASSIGNED,
				   BL_FIA_SATA_LANE07) |
	   BL_FIA_SATA_LANE_CONFIG(BL_ME_FIA_SATA_CONTROLLER_LANE_NOT_ASSIGNED,
				   BL_FIA_SATA_LANE08) |
	   BL_FIA_SATA_LANE_CONFIG(BL_ME_FIA_SATA_CONTROLLER_LANE_NOT_ASSIGNED,
				   BL_FIA_SATA_LANE09) |
	   BL_FIA_SATA_LANE_CONFIG(BL_ME_FIA_SATA_CONTROLLER_LANE_NOT_ASSIGNED,
				   BL_FIA_SATA_LANE10) |
	   BL_FIA_SATA_LANE_CONFIG(BL_ME_FIA_SATA_CONTROLLER_LANE_NOT_ASSIGNED,
				   BL_FIA_SATA_LANE11) |
	   BL_FIA_SATA_LANE_CONFIG(BL_ME_FIA_SATA_CONTROLLER_LANE_NOT_ASSIGNED,
				   BL_FIA_SATA_LANE12) |
	   BL_FIA_SATA_LANE_CONFIG(BL_ME_FIA_SATA_CONTROLLER_LANE_NOT_ASSIGNED,
				   BL_FIA_SATA_LANE13) |
	   BL_FIA_SATA_LANE_CONFIG(BL_ME_FIA_SATA_CONTROLLER_LANE_NOT_ASSIGNED,
				   BL_FIA_SATA_LANE14) |
	   BL_FIA_SATA_LANE_CONFIG(BL_ME_FIA_SATA_CONTROLLER_LANE_NOT_ASSIGNED,
				   BL_FIA_SATA_LANE15) |
	   BL_FIA_SATA_LANE_CONFIG(BL_ME_FIA_SATA_CONTROLLER_LANE_ASSIGNED,
				   BL_FIA_SATA_LANE16) |
	   BL_FIA_SATA_LANE_CONFIG(BL_ME_FIA_SATA_CONTROLLER_LANE_ASSIGNED,
				   BL_FIA_SATA_LANE17) |
	   BL_FIA_SATA_LANE_CONFIG(BL_ME_FIA_SATA_CONTROLLER_LANE_ASSIGNED,
				   BL_FIA_SATA_LANE18) |
	   BL_FIA_SATA_LANE_CONFIG(BL_ME_FIA_SATA_CONTROLLER_LANE_NOT_ASSIGNED,
				   BL_FIA_SATA_LANE19)},

	  /* ME_FIA_PCIE_ROOT_PORTS_CONFIG */
	  {BL_FIA_PCIE_ROOT_PORT_CONFIG(BL_ME_FIA_PCIE_ROOT_PORT_STATE,
					BL_ME_FIA_PCIE_ROOT_PORT_ENABLED,
					BL_FIA_PCIE_ROOT_PORT_0) |
	   BL_FIA_PCIE_ROOT_PORT_CONFIG(BL_ME_FIA_PCIE_ROOT_PORT_STATE,
					BL_ME_FIA_PCIE_ROOT_PORT_DISABLED,
					BL_FIA_PCIE_ROOT_PORT_1) |
	   BL_FIA_PCIE_ROOT_PORT_CONFIG(BL_ME_FIA_PCIE_ROOT_PORT_STATE,
					BL_ME_FIA_PCIE_ROOT_PORT_DISABLED,
					BL_FIA_PCIE_ROOT_PORT_2) |
	   BL_FIA_PCIE_ROOT_PORT_CONFIG(BL_ME_FIA_PCIE_ROOT_PORT_STATE,
					BL_ME_FIA_PCIE_ROOT_PORT_DISABLED,
					BL_FIA_PCIE_ROOT_PORT_3) |
	   BL_FIA_PCIE_ROOT_PORT_CONFIG(BL_ME_FIA_PCIE_ROOT_PORT_STATE,
					BL_ME_FIA_PCIE_ROOT_PORT_ENABLED,
					BL_FIA_PCIE_ROOT_PORT_4) |
	   BL_FIA_PCIE_ROOT_PORT_CONFIG(BL_ME_FIA_PCIE_ROOT_PORT_STATE,
					BL_ME_FIA_PCIE_ROOT_PORT_DISABLED,
					BL_FIA_PCIE_ROOT_PORT_5) |
	   BL_FIA_PCIE_ROOT_PORT_CONFIG(BL_ME_FIA_PCIE_ROOT_PORT_STATE,
					BL_ME_FIA_PCIE_ROOT_PORT_ENABLED,
					BL_FIA_PCIE_ROOT_PORT_6) |
	   BL_FIA_PCIE_ROOT_PORT_CONFIG(BL_ME_FIA_PCIE_ROOT_PORT_STATE,
					BL_ME_FIA_PCIE_ROOT_PORT_DISABLED,
					BL_FIA_PCIE_ROOT_PORT_7) |
	   BL_FIA_PCIE_ROOT_PORT_CONFIG(
		   BL_ME_FIA_PCIE_ROOT_PORT_LINK_WIDTH,
		   BL_ME_FIA_PCIE_ROOT_PORT_LINK_WIDTH_BICTRL,
		   BL_FIA_PCIE_ROOT_PORT_0) |
	   BL_FIA_PCIE_ROOT_PORT_CONFIG(
		   BL_ME_FIA_PCIE_ROOT_PORT_LINK_WIDTH,
		   BL_ME_FIA_PCIE_ROOT_PORT_LINK_WIDTH_BICTRL,
		   BL_FIA_PCIE_ROOT_PORT_1) |
	   BL_FIA_PCIE_ROOT_PORT_CONFIG(
		   BL_ME_FIA_PCIE_ROOT_PORT_LINK_WIDTH,
		   BL_ME_FIA_PCIE_ROOT_PORT_LINK_WIDTH_BICTRL,
		   BL_FIA_PCIE_ROOT_PORT_2) |
	   BL_FIA_PCIE_ROOT_PORT_CONFIG(
		   BL_ME_FIA_PCIE_ROOT_PORT_LINK_WIDTH,
		   BL_ME_FIA_PCIE_ROOT_PORT_LINK_WIDTH_BICTRL,
		   BL_FIA_PCIE_ROOT_PORT_3) |
	   BL_FIA_PCIE_ROOT_PORT_CONFIG(
		   BL_ME_FIA_PCIE_ROOT_PORT_LINK_WIDTH,
		   BL_ME_FIA_PCIE_ROOT_PORT_LINK_WIDTH_BICTRL,
		   BL_FIA_PCIE_ROOT_PORT_4) |
	   BL_FIA_PCIE_ROOT_PORT_CONFIG(
		   BL_ME_FIA_PCIE_ROOT_PORT_LINK_WIDTH,
		   BL_ME_FIA_PCIE_ROOT_PORT_LINK_WIDTH_BICTRL,
		   BL_FIA_PCIE_ROOT_PORT_5) |
	   BL_FIA_PCIE_ROOT_PORT_CONFIG(
		   BL_ME_FIA_PCIE_ROOT_PORT_LINK_WIDTH,
		   BL_ME_FIA_PCIE_ROOT_PORT_LINK_WIDTH_BICTRL,
		   BL_FIA_PCIE_ROOT_PORT_6) |
	   BL_FIA_PCIE_ROOT_PORT_CONFIG(
		   BL_ME_FIA_PCIE_ROOT_PORT_LINK_WIDTH,
		   BL_ME_FIA_PCIE_ROOT_PORT_LINK_WIDTH_BICTRL,
		   BL_FIA_PCIE_ROOT_PORT_7)} } },

	/* SKU HSIO 12 (pcie [0-3, 8-9, 12-13] sata [16-18] USB [19]) */
	{BL_SKU_HSIO_12,
	{PCIE_BIF_CTRL_x4x4, PCIE_BIF_CTRL_x2x2x2x2},
	{/*ME_FIA_MUX_CONFIG */
	  {BL_FIA_LANE_CONFIG(BL_ME_FIA_MUX_LANE_PCIE, BL_FIA_LANE00) |
	   BL_FIA_LANE_CONFIG(BL_ME_FIA_MUX_LANE_PCIE, BL_FIA_LANE01) |
	   BL_FIA_LANE_CONFIG(BL_ME_FIA_MUX_LANE_PCIE, BL_FIA_LANE02) |
	   BL_FIA_LANE_CONFIG(BL_ME_FIA_MUX_LANE_PCIE, BL_FIA_LANE03) |
	   BL_FIA_LANE_CONFIG(BL_ME_FIA_MUX_LANE_DISCONNECTED, BL_FIA_LANE04) |
	   BL_FIA_LANE_CONFIG(BL_ME_FIA_MUX_LANE_DISCONNECTED, BL_FIA_LANE05) |
	   BL_FIA_LANE_CONFIG(BL_ME_FIA_MUX_LANE_DISCONNECTED, BL_FIA_LANE06) |
	   BL_FIA_LANE_CONFIG(BL_ME_FIA_MUX_LANE_DISCONNECTED, BL_FIA_LANE07) |
	   BL_FIA_LANE_CONFIG(BL_ME_FIA_MUX_LANE_PCIE, BL_FIA_LANE08) |
	   BL_FIA_LANE_CONFIG(BL_ME_FIA_MUX_LANE_PCIE, BL_FIA_LANE09) |
	   BL_FIA_LANE_CONFIG(BL_ME_FIA_MUX_LANE_DISCONNECTED, BL_FIA_LANE10) |
	   BL_FIA_LANE_CONFIG(BL_ME_FIA_MUX_LANE_DISCONNECTED, BL_FIA_LANE11) |
	   BL_FIA_LANE_CONFIG(BL_ME_FIA_MUX_LANE_PCIE, BL_FIA_LANE12) |
	   BL_FIA_LANE_CONFIG(BL_ME_FIA_MUX_LANE_PCIE, BL_FIA_LANE13) |
	   BL_FIA_LANE_CONFIG(BL_ME_FIA_MUX_LANE_DISCONNECTED, BL_FIA_LANE14) |
	   BL_FIA_LANE_CONFIG(BL_ME_FIA_MUX_LANE_DISCONNECTED, BL_FIA_LANE15) |
	   BL_FIA_LANE_CONFIG(BL_ME_FIA_MUX_LANE_SATA, BL_FIA_LANE16) |
	   BL_FIA_LANE_CONFIG(BL_ME_FIA_MUX_LANE_SATA, BL_FIA_LANE17) |
	   BL_FIA_LANE_CONFIG(BL_ME_FIA_MUX_LANE_SATA, BL_FIA_LANE18) |
	   BL_FIA_LANE_CONFIG(BL_ME_FIA_MUX_LANE_XHCI, BL_FIA_LANE19)},

	  /* ME_FIA_SATA_CONFIG */
	  {BL_FIA_SATA_LANE_CONFIG(BL_ME_FIA_SATA_CONTROLLER_LANE_NOT_ASSIGNED,
				   BL_FIA_SATA_LANE04) |
	   BL_FIA_SATA_LANE_CONFIG(BL_ME_FIA_SATA_CONTROLLER_LANE_NOT_ASSIGNED,
				   BL_FIA_SATA_LANE05) |
	   BL_FIA_SATA_LANE_CONFIG(BL_ME_FIA_SATA_CONTROLLER_LANE_NOT_ASSIGNED,
				   BL_FIA_SATA_LANE06) |
	   BL_FIA_SATA_LANE_CONFIG(BL_ME_FIA_SATA_CONTROLLER_LANE_NOT_ASSIGNED,
				   BL_FIA_SATA_LANE07) |
	   BL_FIA_SATA_LANE_CONFIG(BL_ME_FIA_SATA_CONTROLLER_LANE_NOT_ASSIGNED,
				   BL_FIA_SATA_LANE08) |
	   BL_FIA_SATA_LANE_CONFIG(BL_ME_FIA_SATA_CONTROLLER_LANE_NOT_ASSIGNED,
				   BL_FIA_SATA_LANE09) |
	   BL_FIA_SATA_LANE_CONFIG(BL_ME_FIA_SATA_CONTROLLER_LANE_NOT_ASSIGNED,
				   BL_FIA_SATA_LANE10) |
	   BL_FIA_SATA_LANE_CONFIG(BL_ME_FIA_SATA_CONTROLLER_LANE_NOT_ASSIGNED,
				   BL_FIA_SATA_LANE11) |
	   BL_FIA_SATA_LANE_CONFIG(BL_ME_FIA_SATA_CONTROLLER_LANE_NOT_ASSIGNED,
				   BL_FIA_SATA_LANE12) |
	   BL_FIA_SATA_LANE_CONFIG(BL_ME_FIA_SATA_CONTROLLER_LANE_NOT_ASSIGNED,
				   BL_FIA_SATA_LANE13) |
	   BL_FIA_SATA_LANE_CONFIG(BL_ME_FIA_SATA_CONTROLLER_LANE_NOT_ASSIGNED,
				   BL_FIA_SATA_LANE14) |
	   BL_FIA_SATA_LANE_CONFIG(BL_ME_FIA_SATA_CONTROLLER_LANE_NOT_ASSIGNED,
				   BL_FIA_SATA_LANE15) |
	   BL_FIA_SATA_LANE_CONFIG(BL_ME_FIA_SATA_CONTROLLER_LANE_ASSIGNED,
				   BL_FIA_SATA_LANE16) |
	   BL_FIA_SATA_LANE_CONFIG(BL_ME_FIA_SATA_CONTROLLER_LANE_ASSIGNED,
				   BL_FIA_SATA_LANE17) |
	   BL_FIA_SATA_LANE_CONFIG(BL_ME_FIA_SATA_CONTROLLER_LANE_ASSIGNED,
				   BL_FIA_SATA_LANE18) |
	   BL_FIA_SATA_LANE_CONFIG(BL_ME_FIA_SATA_CONTROLLER_LANE_NOT_ASSIGNED,
				   BL_FIA_SATA_LANE19)},

	  /* ME_FIA_PCIE_ROOT_PORTS_CONFIG */
	  {BL_FIA_PCIE_ROOT_PORT_CONFIG(BL_ME_FIA_PCIE_ROOT_PORT_STATE,
					BL_ME_FIA_PCIE_ROOT_PORT_ENABLED,
					BL_FIA_PCIE_ROOT_PORT_0) |
	   BL_FIA_PCIE_ROOT_PORT_CONFIG(BL_ME_FIA_PCIE_ROOT_PORT_STATE,
					BL_ME_FIA_PCIE_ROOT_PORT_DISABLED,
					BL_FIA_PCIE_ROOT_PORT_1) |
	   BL_FIA_PCIE_ROOT_PORT_CONFIG(BL_ME_FIA_PCIE_ROOT_PORT_STATE,
					BL_ME_FIA_PCIE_ROOT_PORT_DISABLED,
					BL_FIA_PCIE_ROOT_PORT_2) |
	   BL_FIA_PCIE_ROOT_PORT_CONFIG(BL_ME_FIA_PCIE_ROOT_PORT_STATE,
					BL_ME_FIA_PCIE_ROOT_PORT_DISABLED,
					BL_FIA_PCIE_ROOT_PORT_3) |
	   BL_FIA_PCIE_ROOT_PORT_CONFIG(BL_ME_FIA_PCIE_ROOT_PORT_STATE,
					BL_ME_FIA_PCIE_ROOT_PORT_ENABLED,
					BL_FIA_PCIE_ROOT_PORT_4) |
	   BL_FIA_PCIE_ROOT_PORT_CONFIG(BL_ME_FIA_PCIE_ROOT_PORT_STATE,
					BL_ME_FIA_PCIE_ROOT_PORT_DISABLED,
					BL_FIA_PCIE_ROOT_PORT_5) |
	   BL_FIA_PCIE_ROOT_PORT_CONFIG(BL_ME_FIA_PCIE_ROOT_PORT_STATE,
					BL_ME_FIA_PCIE_ROOT_PORT_ENABLED,
					BL_FIA_PCIE_ROOT_PORT_6) |
	   BL_FIA_PCIE_ROOT_PORT_CONFIG(BL_ME_FIA_PCIE_ROOT_PORT_STATE,
					BL_ME_FIA_PCIE_ROOT_PORT_DISABLED,
					BL_FIA_PCIE_ROOT_PORT_7) |
	   BL_FIA_PCIE_ROOT_PORT_CONFIG(
		   BL_ME_FIA_PCIE_ROOT_PORT_LINK_WIDTH,
		   BL_ME_FIA_PCIE_ROOT_PORT_LINK_WIDTH_BICTRL,
		   BL_FIA_PCIE_ROOT_PORT_0) |
	   BL_FIA_PCIE_ROOT_PORT_CONFIG(
		   BL_ME_FIA_PCIE_ROOT_PORT_LINK_WIDTH,
		   BL_ME_FIA_PCIE_ROOT_PORT_LINK_WIDTH_BICTRL,
		   BL_FIA_PCIE_ROOT_PORT_1) |
	   BL_FIA_PCIE_ROOT_PORT_CONFIG(
		   BL_ME_FIA_PCIE_ROOT_PORT_LINK_WIDTH,
		   BL_ME_FIA_PCIE_ROOT_PORT_LINK_WIDTH_BICTRL,
		   BL_FIA_PCIE_ROOT_PORT_2) |
	   BL_FIA_PCIE_ROOT_PORT_CONFIG(
		   BL_ME_FIA_PCIE_ROOT_PORT_LINK_WIDTH,
		   BL_ME_FIA_PCIE_ROOT_PORT_LINK_WIDTH_BICTRL,
		   BL_FIA_PCIE_ROOT_PORT_3) |
	   BL_FIA_PCIE_ROOT_PORT_CONFIG(
		   BL_ME_FIA_PCIE_ROOT_PORT_LINK_WIDTH,
		   BL_ME_FIA_PCIE_ROOT_PORT_LINK_WIDTH_BICTRL,
		   BL_FIA_PCIE_ROOT_PORT_4) |
	   BL_FIA_PCIE_ROOT_PORT_CONFIG(
		   BL_ME_FIA_PCIE_ROOT_PORT_LINK_WIDTH,
		   BL_ME_FIA_PCIE_ROOT_PORT_LINK_WIDTH_BICTRL,
		   BL_FIA_PCIE_ROOT_PORT_5) |
	   BL_FIA_PCIE_ROOT_PORT_CONFIG(
		   BL_ME_FIA_PCIE_ROOT_PORT_LINK_WIDTH,
		   BL_ME_FIA_PCIE_ROOT_PORT_LINK_WIDTH_BICTRL,
		   BL_FIA_PCIE_ROOT_PORT_6) |
	   BL_FIA_PCIE_ROOT_PORT_CONFIG(
		   BL_ME_FIA_PCIE_ROOT_PORT_LINK_WIDTH,
		   BL_ME_FIA_PCIE_ROOT_PORT_LINK_WIDTH_BICTRL,
		   BL_FIA_PCIE_ROOT_PORT_7)} } },

	/* SKU HSIO 10 (pcie [0-3, 8-9, 12] sata [16-17] USB [19]) */
	{BL_SKU_HSIO_10,
	{PCIE_BIF_CTRL_x4x4, PCIE_BIF_CTRL_x2x2x2x2},
	{/* ME_FIA_MUX_CONFIG */
	  {BL_FIA_LANE_CONFIG(BL_ME_FIA_MUX_LANE_PCIE, BL_FIA_LANE00) |
	   BL_FIA_LANE_CONFIG(BL_ME_FIA_MUX_LANE_PCIE, BL_FIA_LANE01) |
	   BL_FIA_LANE_CONFIG(BL_ME_FIA_MUX_LANE_PCIE, BL_FIA_LANE02) |
	   BL_FIA_LANE_CONFIG(BL_ME_FIA_MUX_LANE_PCIE, BL_FIA_LANE03) |
	   BL_FIA_LANE_CONFIG(BL_ME_FIA_MUX_LANE_DISCONNECTED, BL_FIA_LANE04) |
	   BL_FIA_LANE_CONFIG(BL_ME_FIA_MUX_LANE_DISCONNECTED, BL_FIA_LANE05) |
	   BL_FIA_LANE_CONFIG(BL_ME_FIA_MUX_LANE_DISCONNECTED, BL_FIA_LANE06) |
	   BL_FIA_LANE_CONFIG(BL_ME_FIA_MUX_LANE_DISCONNECTED, BL_FIA_LANE07) |
	   BL_FIA_LANE_CONFIG(BL_ME_FIA_MUX_LANE_PCIE, BL_FIA_LANE08) |
	   BL_FIA_LANE_CONFIG(BL_ME_FIA_MUX_LANE_PCIE, BL_FIA_LANE09) |
	   BL_FIA_LANE_CONFIG(BL_ME_FIA_MUX_LANE_DISCONNECTED, BL_FIA_LANE10) |
	   BL_FIA_LANE_CONFIG(BL_ME_FIA_MUX_LANE_DISCONNECTED, BL_FIA_LANE11) |
	   BL_FIA_LANE_CONFIG(BL_ME_FIA_MUX_LANE_PCIE, BL_FIA_LANE12) |
	   BL_FIA_LANE_CONFIG(BL_ME_FIA_MUX_LANE_DISCONNECTED, BL_FIA_LANE13) |
	   BL_FIA_LANE_CONFIG(BL_ME_FIA_MUX_LANE_DISCONNECTED, BL_FIA_LANE14) |
	   BL_FIA_LANE_CONFIG(BL_ME_FIA_MUX_LANE_DISCONNECTED, BL_FIA_LANE15) |
	   BL_FIA_LANE_CONFIG(BL_ME_FIA_MUX_LANE_SATA, BL_FIA_LANE16) |
	   BL_FIA_LANE_CONFIG(BL_ME_FIA_MUX_LANE_SATA, BL_FIA_LANE17) |
	   BL_FIA_LANE_CONFIG(BL_ME_FIA_MUX_LANE_DISCONNECTED, BL_FIA_LANE18) |
	   BL_FIA_LANE_CONFIG(BL_ME_FIA_MUX_LANE_XHCI, BL_FIA_LANE19)},

	  /* ME_FIA_SATA_CONFIG */
	  {BL_FIA_SATA_LANE_CONFIG(BL_ME_FIA_SATA_CONTROLLER_LANE_NOT_ASSIGNED,
				   BL_FIA_SATA_LANE04) |
	   BL_FIA_SATA_LANE_CONFIG(BL_ME_FIA_SATA_CONTROLLER_LANE_NOT_ASSIGNED,
				   BL_FIA_SATA_LANE05) |
	   BL_FIA_SATA_LANE_CONFIG(BL_ME_FIA_SATA_CONTROLLER_LANE_NOT_ASSIGNED,
				   BL_FIA_SATA_LANE06) |
	   BL_FIA_SATA_LANE_CONFIG(BL_ME_FIA_SATA_CONTROLLER_LANE_NOT_ASSIGNED,
				   BL_FIA_SATA_LANE07) |
	   BL_FIA_SATA_LANE_CONFIG(BL_ME_FIA_SATA_CONTROLLER_LANE_NOT_ASSIGNED,
				   BL_FIA_SATA_LANE08) |
	   BL_FIA_SATA_LANE_CONFIG(BL_ME_FIA_SATA_CONTROLLER_LANE_NOT_ASSIGNED,
				   BL_FIA_SATA_LANE09) |
	   BL_FIA_SATA_LANE_CONFIG(BL_ME_FIA_SATA_CONTROLLER_LANE_NOT_ASSIGNED,
				   BL_FIA_SATA_LANE10) |
	   BL_FIA_SATA_LANE_CONFIG(BL_ME_FIA_SATA_CONTROLLER_LANE_NOT_ASSIGNED,
				   BL_FIA_SATA_LANE11) |
	   BL_FIA_SATA_LANE_CONFIG(BL_ME_FIA_SATA_CONTROLLER_LANE_NOT_ASSIGNED,
				   BL_FIA_SATA_LANE12) |
	   BL_FIA_SATA_LANE_CONFIG(BL_ME_FIA_SATA_CONTROLLER_LANE_NOT_ASSIGNED,
				   BL_FIA_SATA_LANE13) |
	   BL_FIA_SATA_LANE_CONFIG(BL_ME_FIA_SATA_CONTROLLER_LANE_NOT_ASSIGNED,
				   BL_FIA_SATA_LANE14) |
	   BL_FIA_SATA_LANE_CONFIG(BL_ME_FIA_SATA_CONTROLLER_LANE_NOT_ASSIGNED,
				   BL_FIA_SATA_LANE15) |
	   BL_FIA_SATA_LANE_CONFIG(BL_ME_FIA_SATA_CONTROLLER_LANE_ASSIGNED,
				   BL_FIA_SATA_LANE16) |
	   BL_FIA_SATA_LANE_CONFIG(BL_ME_FIA_SATA_CONTROLLER_LANE_ASSIGNED,
				   BL_FIA_SATA_LANE17) |
	   BL_FIA_SATA_LANE_CONFIG(BL_ME_FIA_SATA_CONTROLLER_LANE_NOT_ASSIGNED,
				   BL_FIA_SATA_LANE18) |
	   BL_FIA_SATA_LANE_CONFIG(BL_ME_FIA_SATA_CONTROLLER_LANE_NOT_ASSIGNED,
				   BL_FIA_SATA_LANE19)},

	  /* ME_FIA_PCIE_ROOT_PORTS_CONFIG */
	  {BL_FIA_PCIE_ROOT_PORT_CONFIG(BL_ME_FIA_PCIE_ROOT_PORT_STATE,
					BL_ME_FIA_PCIE_ROOT_PORT_ENABLED,
					BL_FIA_PCIE_ROOT_PORT_0) |
	   BL_FIA_PCIE_ROOT_PORT_CONFIG(BL_ME_FIA_PCIE_ROOT_PORT_STATE,
					BL_ME_FIA_PCIE_ROOT_PORT_DISABLED,
					BL_FIA_PCIE_ROOT_PORT_1) |
	   BL_FIA_PCIE_ROOT_PORT_CONFIG(BL_ME_FIA_PCIE_ROOT_PORT_STATE,
					BL_ME_FIA_PCIE_ROOT_PORT_DISABLED,
					BL_FIA_PCIE_ROOT_PORT_2) |
	   BL_FIA_PCIE_ROOT_PORT_CONFIG(BL_ME_FIA_PCIE_ROOT_PORT_STATE,
					BL_ME_FIA_PCIE_ROOT_PORT_DISABLED,
					BL_FIA_PCIE_ROOT_PORT_3) |
	   BL_FIA_PCIE_ROOT_PORT_CONFIG(BL_ME_FIA_PCIE_ROOT_PORT_STATE,
					BL_ME_FIA_PCIE_ROOT_PORT_ENABLED,
					BL_FIA_PCIE_ROOT_PORT_4) |
	   BL_FIA_PCIE_ROOT_PORT_CONFIG(BL_ME_FIA_PCIE_ROOT_PORT_STATE,
					BL_ME_FIA_PCIE_ROOT_PORT_DISABLED,
					BL_FIA_PCIE_ROOT_PORT_5) |
	   BL_FIA_PCIE_ROOT_PORT_CONFIG(BL_ME_FIA_PCIE_ROOT_PORT_STATE,
					BL_ME_FIA_PCIE_ROOT_PORT_ENABLED,
					BL_FIA_PCIE_ROOT_PORT_6) |
	   BL_FIA_PCIE_ROOT_PORT_CONFIG(BL_ME_FIA_PCIE_ROOT_PORT_STATE,
					BL_ME_FIA_PCIE_ROOT_PORT_DISABLED,
					BL_FIA_PCIE_ROOT_PORT_7) |
	   BL_FIA_PCIE_ROOT_PORT_CONFIG(
		   BL_ME_FIA_PCIE_ROOT_PORT_LINK_WIDTH,
		   BL_ME_FIA_PCIE_ROOT_PORT_LINK_WIDTH_BICTRL,
		   BL_FIA_PCIE_ROOT_PORT_0) |
	   BL_FIA_PCIE_ROOT_PORT_CONFIG(
		   BL_ME_FIA_PCIE_ROOT_PORT_LINK_WIDTH,
		   BL_ME_FIA_PCIE_ROOT_PORT_LINK_WIDTH_BICTRL,
		   BL_FIA_PCIE_ROOT_PORT_1) |
	   BL_FIA_PCIE_ROOT_PORT_CONFIG(
		   BL_ME_FIA_PCIE_ROOT_PORT_LINK_WIDTH,
		   BL_ME_FIA_PCIE_ROOT_PORT_LINK_WIDTH_BICTRL,
		   BL_FIA_PCIE_ROOT_PORT_2) |
	   BL_FIA_PCIE_ROOT_PORT_CONFIG(
		   BL_ME_FIA_PCIE_ROOT_PORT_LINK_WIDTH,
		   BL_ME_FIA_PCIE_ROOT_PORT_LINK_WIDTH_BICTRL,
		   BL_FIA_PCIE_ROOT_PORT_3) |
	   BL_FIA_PCIE_ROOT_PORT_CONFIG(
		   BL_ME_FIA_PCIE_ROOT_PORT_LINK_WIDTH,
		   BL_ME_FIA_PCIE_ROOT_PORT_LINK_WIDTH_BICTRL,
		   BL_FIA_PCIE_ROOT_PORT_4) |
	   BL_FIA_PCIE_ROOT_PORT_CONFIG(
		   BL_ME_FIA_PCIE_ROOT_PORT_LINK_WIDTH,
		   BL_ME_FIA_PCIE_ROOT_PORT_LINK_WIDTH_BICTRL,
		   BL_FIA_PCIE_ROOT_PORT_5) |
	   BL_FIA_PCIE_ROOT_PORT_CONFIG(BL_ME_FIA_PCIE_ROOT_PORT_LINK_WIDTH,
					BL_ME_FIA_PCIE_ROOT_PORT_LINK_X1,
					BL_FIA_PCIE_ROOT_PORT_6) |
	   BL_FIA_PCIE_ROOT_PORT_CONFIG(
		   BL_ME_FIA_PCIE_ROOT_PORT_LINK_WIDTH,
		   BL_ME_FIA_PCIE_ROOT_PORT_LINK_WIDTH_BICTRL,
		   BL_FIA_PCIE_ROOT_PORT_7)} } },

	/* SKU HSIO 8 (pcie [0-1, 8-9, 12] sata [16-17] USB [19]) */
	{BL_SKU_HSIO_08,
	{PCIE_BIF_CTRL_x2x2x2x2, PCIE_BIF_CTRL_x2x2x2x2},
	{/* ME_FIA_MUX_CONFIG */
	  {BL_FIA_LANE_CONFIG(BL_ME_FIA_MUX_LANE_PCIE, BL_FIA_LANE00) |
	   BL_FIA_LANE_CONFIG(BL_ME_FIA_MUX_LANE_PCIE, BL_FIA_LANE01) |
	   BL_FIA_LANE_CONFIG(BL_ME_FIA_MUX_LANE_DISCONNECTED, BL_FIA_LANE02) |
	   BL_FIA_LANE_CONFIG(BL_ME_FIA_MUX_LANE_DISCONNECTED, BL_FIA_LANE03) |
	   BL_FIA_LANE_CONFIG(BL_ME_FIA_MUX_LANE_DISCONNECTED, BL_FIA_LANE04) |
	   BL_FIA_LANE_CONFIG(BL_ME_FIA_MUX_LANE_DISCONNECTED, BL_FIA_LANE05) |
	   BL_FIA_LANE_CONFIG(BL_ME_FIA_MUX_LANE_DISCONNECTED, BL_FIA_LANE06) |
	   BL_FIA_LANE_CONFIG(BL_ME_FIA_MUX_LANE_DISCONNECTED, BL_FIA_LANE07) |
	   BL_FIA_LANE_CONFIG(BL_ME_FIA_MUX_LANE_PCIE, BL_FIA_LANE08) |
	   BL_FIA_LANE_CONFIG(BL_ME_FIA_MUX_LANE_PCIE, BL_FIA_LANE09) |
	   BL_FIA_LANE_CONFIG(BL_ME_FIA_MUX_LANE_DISCONNECTED, BL_FIA_LANE10) |
	   BL_FIA_LANE_CONFIG(BL_ME_FIA_MUX_LANE_DISCONNECTED, BL_FIA_LANE11) |
	   BL_FIA_LANE_CONFIG(BL_ME_FIA_MUX_LANE_PCIE, BL_FIA_LANE12) |
	   BL_FIA_LANE_CONFIG(BL_ME_FIA_MUX_LANE_DISCONNECTED, BL_FIA_LANE13) |
	   BL_FIA_LANE_CONFIG(BL_ME_FIA_MUX_LANE_DISCONNECTED, BL_FIA_LANE14) |
	   BL_FIA_LANE_CONFIG(BL_ME_FIA_MUX_LANE_DISCONNECTED, BL_FIA_LANE15) |
	   BL_FIA_LANE_CONFIG(BL_ME_FIA_MUX_LANE_SATA, BL_FIA_LANE16) |
	   BL_FIA_LANE_CONFIG(BL_ME_FIA_MUX_LANE_SATA, BL_FIA_LANE17) |
	   BL_FIA_LANE_CONFIG(BL_ME_FIA_MUX_LANE_DISCONNECTED, BL_FIA_LANE18) |
	   BL_FIA_LANE_CONFIG(BL_ME_FIA_MUX_LANE_XHCI, BL_FIA_LANE19)},

	  /* ME_FIA_SATA_CONFIG */
	  {BL_FIA_SATA_LANE_CONFIG(BL_ME_FIA_SATA_CONTROLLER_LANE_NOT_ASSIGNED,
				   BL_FIA_SATA_LANE04) |
	   BL_FIA_SATA_LANE_CONFIG(BL_ME_FIA_SATA_CONTROLLER_LANE_NOT_ASSIGNED,
				   BL_FIA_SATA_LANE05) |
	   BL_FIA_SATA_LANE_CONFIG(BL_ME_FIA_SATA_CONTROLLER_LANE_NOT_ASSIGNED,
				   BL_FIA_SATA_LANE06) |
	   BL_FIA_SATA_LANE_CONFIG(BL_ME_FIA_SATA_CONTROLLER_LANE_NOT_ASSIGNED,
				   BL_FIA_SATA_LANE07) |
	   BL_FIA_SATA_LANE_CONFIG(BL_ME_FIA_SATA_CONTROLLER_LANE_NOT_ASSIGNED,
				   BL_FIA_SATA_LANE08) |
	   BL_FIA_SATA_LANE_CONFIG(BL_ME_FIA_SATA_CONTROLLER_LANE_NOT_ASSIGNED,
				   BL_FIA_SATA_LANE09) |
	   BL_FIA_SATA_LANE_CONFIG(BL_ME_FIA_SATA_CONTROLLER_LANE_NOT_ASSIGNED,
				   BL_FIA_SATA_LANE10) |
	   BL_FIA_SATA_LANE_CONFIG(BL_ME_FIA_SATA_CONTROLLER_LANE_NOT_ASSIGNED,
				   BL_FIA_SATA_LANE11) |
	   BL_FIA_SATA_LANE_CONFIG(BL_ME_FIA_SATA_CONTROLLER_LANE_NOT_ASSIGNED,
				   BL_FIA_SATA_LANE12) |
	   BL_FIA_SATA_LANE_CONFIG(BL_ME_FIA_SATA_CONTROLLER_LANE_NOT_ASSIGNED,
				   BL_FIA_SATA_LANE13) |
	   BL_FIA_SATA_LANE_CONFIG(BL_ME_FIA_SATA_CONTROLLER_LANE_NOT_ASSIGNED,
				   BL_FIA_SATA_LANE14) |
	   BL_FIA_SATA_LANE_CONFIG(BL_ME_FIA_SATA_CONTROLLER_LANE_NOT_ASSIGNED,
				   BL_FIA_SATA_LANE15) |
	   BL_FIA_SATA_LANE_CONFIG(BL_ME_FIA_SATA_CONTROLLER_LANE_ASSIGNED,
				   BL_FIA_SATA_LANE16) |
	   BL_FIA_SATA_LANE_CONFIG(BL_ME_FIA_SATA_CONTROLLER_LANE_ASSIGNED,
				   BL_FIA_SATA_LANE17) |
	   BL_FIA_SATA_LANE_CONFIG(BL_ME_FIA_SATA_CONTROLLER_LANE_NOT_ASSIGNED,
				   BL_FIA_SATA_LANE18) |
	   BL_FIA_SATA_LANE_CONFIG(BL_ME_FIA_SATA_CONTROLLER_LANE_NOT_ASSIGNED,
				   BL_FIA_SATA_LANE19)},

	  /* ME_FIA_PCIE_ROOT_PORTS_CONFIG */
	  {BL_FIA_PCIE_ROOT_PORT_CONFIG(BL_ME_FIA_PCIE_ROOT_PORT_STATE,
					BL_ME_FIA_PCIE_ROOT_PORT_ENABLED,
					BL_FIA_PCIE_ROOT_PORT_0) |
	   BL_FIA_PCIE_ROOT_PORT_CONFIG(BL_ME_FIA_PCIE_ROOT_PORT_STATE,
					BL_ME_FIA_PCIE_ROOT_PORT_DISABLED,
					BL_FIA_PCIE_ROOT_PORT_1) |
	   BL_FIA_PCIE_ROOT_PORT_CONFIG(BL_ME_FIA_PCIE_ROOT_PORT_STATE,
					BL_ME_FIA_PCIE_ROOT_PORT_DISABLED,
					BL_FIA_PCIE_ROOT_PORT_2) |
	   BL_FIA_PCIE_ROOT_PORT_CONFIG(BL_ME_FIA_PCIE_ROOT_PORT_STATE,
					BL_ME_FIA_PCIE_ROOT_PORT_DISABLED,
					BL_FIA_PCIE_ROOT_PORT_3) |
	   BL_FIA_PCIE_ROOT_PORT_CONFIG(BL_ME_FIA_PCIE_ROOT_PORT_STATE,
					BL_ME_FIA_PCIE_ROOT_PORT_ENABLED,
					BL_FIA_PCIE_ROOT_PORT_4) |
	   BL_FIA_PCIE_ROOT_PORT_CONFIG(BL_ME_FIA_PCIE_ROOT_PORT_STATE,
					BL_ME_FIA_PCIE_ROOT_PORT_DISABLED,
					BL_FIA_PCIE_ROOT_PORT_5) |
	   BL_FIA_PCIE_ROOT_PORT_CONFIG(BL_ME_FIA_PCIE_ROOT_PORT_STATE,
					BL_ME_FIA_PCIE_ROOT_PORT_ENABLED,
					BL_FIA_PCIE_ROOT_PORT_6) |
	   BL_FIA_PCIE_ROOT_PORT_CONFIG(BL_ME_FIA_PCIE_ROOT_PORT_STATE,
					BL_ME_FIA_PCIE_ROOT_PORT_DISABLED,
					BL_FIA_PCIE_ROOT_PORT_7) |
	   BL_FIA_PCIE_ROOT_PORT_CONFIG(
		   BL_ME_FIA_PCIE_ROOT_PORT_LINK_WIDTH,
		   BL_ME_FIA_PCIE_ROOT_PORT_LINK_WIDTH_BICTRL,
		   BL_FIA_PCIE_ROOT_PORT_0) |
	   BL_FIA_PCIE_ROOT_PORT_CONFIG(
		   BL_ME_FIA_PCIE_ROOT_PORT_LINK_WIDTH,
		   BL_ME_FIA_PCIE_ROOT_PORT_LINK_WIDTH_BICTRL,
		   BL_FIA_PCIE_ROOT_PORT_1) |
	   BL_FIA_PCIE_ROOT_PORT_CONFIG(
		   BL_ME_FIA_PCIE_ROOT_PORT_LINK_WIDTH,
		   BL_ME_FIA_PCIE_ROOT_PORT_LINK_WIDTH_BICTRL,
		   BL_FIA_PCIE_ROOT_PORT_2) |
	   BL_FIA_PCIE_ROOT_PORT_CONFIG(
		   BL_ME_FIA_PCIE_ROOT_PORT_LINK_WIDTH,
		   BL_ME_FIA_PCIE_ROOT_PORT_LINK_WIDTH_BICTRL,
		   BL_FIA_PCIE_ROOT_PORT_3) |
	   BL_FIA_PCIE_ROOT_PORT_CONFIG(
		   BL_ME_FIA_PCIE_ROOT_PORT_LINK_WIDTH,
		   BL_ME_FIA_PCIE_ROOT_PORT_LINK_WIDTH_BICTRL,
		   BL_FIA_PCIE_ROOT_PORT_4) |
	   BL_FIA_PCIE_ROOT_PORT_CONFIG(
		   BL_ME_FIA_PCIE_ROOT_PORT_LINK_WIDTH,
		   BL_ME_FIA_PCIE_ROOT_PORT_LINK_WIDTH_BICTRL,
		   BL_FIA_PCIE_ROOT_PORT_5) |
	   BL_FIA_PCIE_ROOT_PORT_CONFIG(BL_ME_FIA_PCIE_ROOT_PORT_LINK_WIDTH,
					BL_ME_FIA_PCIE_ROOT_PORT_LINK_X1,
					BL_FIA_PCIE_ROOT_PORT_6) |
	   BL_FIA_PCIE_ROOT_PORT_CONFIG(
		   BL_ME_FIA_PCIE_ROOT_PORT_LINK_WIDTH,
		   BL_ME_FIA_PCIE_ROOT_PORT_LINK_WIDTH_BICTRL,
		   BL_FIA_PCIE_ROOT_PORT_7)} } },

	/* SKU HSIO 6 (pcie [0-1, 8, 12] sata [16] USB [19]) */
	{BL_SKU_HSIO_06,
	{PCIE_BIF_CTRL_x2x2x2x2, PCIE_BIF_CTRL_x2x2x2x2},
	{/* ME_FIA_MUX_CONFIG */
	  {BL_FIA_LANE_CONFIG(BL_ME_FIA_MUX_LANE_PCIE, BL_FIA_LANE00) |
	   BL_FIA_LANE_CONFIG(BL_ME_FIA_MUX_LANE_PCIE, BL_FIA_LANE01) |
	   BL_FIA_LANE_CONFIG(BL_ME_FIA_MUX_LANE_DISCONNECTED, BL_FIA_LANE02) |
	   BL_FIA_LANE_CONFIG(BL_ME_FIA_MUX_LANE_DISCONNECTED, BL_FIA_LANE03) |
	   BL_FIA_LANE_CONFIG(BL_ME_FIA_MUX_LANE_DISCONNECTED, BL_FIA_LANE04) |
	   BL_FIA_LANE_CONFIG(BL_ME_FIA_MUX_LANE_DISCONNECTED, BL_FIA_LANE05) |
	   BL_FIA_LANE_CONFIG(BL_ME_FIA_MUX_LANE_DISCONNECTED, BL_FIA_LANE06) |
	   BL_FIA_LANE_CONFIG(BL_ME_FIA_MUX_LANE_DISCONNECTED, BL_FIA_LANE07) |
	   BL_FIA_LANE_CONFIG(BL_ME_FIA_MUX_LANE_PCIE, BL_FIA_LANE08) |
	   BL_FIA_LANE_CONFIG(BL_ME_FIA_MUX_LANE_DISCONNECTED, BL_FIA_LANE09) |
	   BL_FIA_LANE_CONFIG(BL_ME_FIA_MUX_LANE_DISCONNECTED, BL_FIA_LANE10) |
	   BL_FIA_LANE_CONFIG(BL_ME_FIA_MUX_LANE_DISCONNECTED, BL_FIA_LANE11) |
	   BL_FIA_LANE_CONFIG(BL_ME_FIA_MUX_LANE_PCIE, BL_FIA_LANE12) |
	   BL_FIA_LANE_CONFIG(BL_ME_FIA_MUX_LANE_DISCONNECTED, BL_FIA_LANE13) |
	   BL_FIA_LANE_CONFIG(BL_ME_FIA_MUX_LANE_DISCONNECTED, BL_FIA_LANE14) |
	   BL_FIA_LANE_CONFIG(BL_ME_FIA_MUX_LANE_DISCONNECTED, BL_FIA_LANE15) |
	   BL_FIA_LANE_CONFIG(BL_ME_FIA_MUX_LANE_SATA, BL_FIA_LANE16) |
	   BL_FIA_LANE_CONFIG(BL_ME_FIA_MUX_LANE_DISCONNECTED, BL_FIA_LANE17) |
	   BL_FIA_LANE_CONFIG(BL_ME_FIA_MUX_LANE_DISCONNECTED, BL_FIA_LANE18) |
	   BL_FIA_LANE_CONFIG(BL_ME_FIA_MUX_LANE_XHCI, BL_FIA_LANE19)},

	  /* ME_FIA_SATA_CONFIG */
	  {BL_FIA_SATA_LANE_CONFIG(BL_ME_FIA_SATA_CONTROLLER_LANE_NOT_ASSIGNED,
				   BL_FIA_SATA_LANE04) |
	   BL_FIA_SATA_LANE_CONFIG(BL_ME_FIA_SATA_CONTROLLER_LANE_NOT_ASSIGNED,
				   BL_FIA_SATA_LANE05) |
	   BL_FIA_SATA_LANE_CONFIG(BL_ME_FIA_SATA_CONTROLLER_LANE_NOT_ASSIGNED,
				   BL_FIA_SATA_LANE06) |
	   BL_FIA_SATA_LANE_CONFIG(BL_ME_FIA_SATA_CONTROLLER_LANE_NOT_ASSIGNED,
				   BL_FIA_SATA_LANE07) |
	   BL_FIA_SATA_LANE_CONFIG(BL_ME_FIA_SATA_CONTROLLER_LANE_NOT_ASSIGNED,
				   BL_FIA_SATA_LANE08) |
	   BL_FIA_SATA_LANE_CONFIG(BL_ME_FIA_SATA_CONTROLLER_LANE_NOT_ASSIGNED,
				   BL_FIA_SATA_LANE09) |
	   BL_FIA_SATA_LANE_CONFIG(BL_ME_FIA_SATA_CONTROLLER_LANE_NOT_ASSIGNED,
				   BL_FIA_SATA_LANE10) |
	   BL_FIA_SATA_LANE_CONFIG(BL_ME_FIA_SATA_CONTROLLER_LANE_NOT_ASSIGNED,
				   BL_FIA_SATA_LANE11) |
	   BL_FIA_SATA_LANE_CONFIG(BL_ME_FIA_SATA_CONTROLLER_LANE_NOT_ASSIGNED,
				   BL_FIA_SATA_LANE12) |
	   BL_FIA_SATA_LANE_CONFIG(BL_ME_FIA_SATA_CONTROLLER_LANE_NOT_ASSIGNED,
				   BL_FIA_SATA_LANE13) |
	   BL_FIA_SATA_LANE_CONFIG(BL_ME_FIA_SATA_CONTROLLER_LANE_NOT_ASSIGNED,
				   BL_FIA_SATA_LANE14) |
	   BL_FIA_SATA_LANE_CONFIG(BL_ME_FIA_SATA_CONTROLLER_LANE_NOT_ASSIGNED,
				   BL_FIA_SATA_LANE15) |
	   BL_FIA_SATA_LANE_CONFIG(BL_ME_FIA_SATA_CONTROLLER_LANE_ASSIGNED,
				   BL_FIA_SATA_LANE16) |
	   BL_FIA_SATA_LANE_CONFIG(BL_ME_FIA_SATA_CONTROLLER_LANE_NOT_ASSIGNED,
				   BL_FIA_SATA_LANE17) |
	   BL_FIA_SATA_LANE_CONFIG(BL_ME_FIA_SATA_CONTROLLER_LANE_NOT_ASSIGNED,
				   BL_FIA_SATA_LANE18) |
	   BL_FIA_SATA_LANE_CONFIG(BL_ME_FIA_SATA_CONTROLLER_LANE_NOT_ASSIGNED,
				   BL_FIA_SATA_LANE19)},

	  /* ME_FIA_PCIE_ROOT_PORTS_CONFIG */
	  {BL_FIA_PCIE_ROOT_PORT_CONFIG(BL_ME_FIA_PCIE_ROOT_PORT_STATE,
					BL_ME_FIA_PCIE_ROOT_PORT_ENABLED,
					BL_FIA_PCIE_ROOT_PORT_0) |
	   BL_FIA_PCIE_ROOT_PORT_CONFIG(BL_ME_FIA_PCIE_ROOT_PORT_STATE,
					BL_ME_FIA_PCIE_ROOT_PORT_DISABLED,
					BL_FIA_PCIE_ROOT_PORT_1) |
	   BL_FIA_PCIE_ROOT_PORT_CONFIG(BL_ME_FIA_PCIE_ROOT_PORT_STATE,
					BL_ME_FIA_PCIE_ROOT_PORT_DISABLED,
					BL_FIA_PCIE_ROOT_PORT_2) |
	   BL_FIA_PCIE_ROOT_PORT_CONFIG(BL_ME_FIA_PCIE_ROOT_PORT_STATE,
					BL_ME_FIA_PCIE_ROOT_PORT_DISABLED,
					BL_FIA_PCIE_ROOT_PORT_3) |
	   BL_FIA_PCIE_ROOT_PORT_CONFIG(BL_ME_FIA_PCIE_ROOT_PORT_STATE,
					BL_ME_FIA_PCIE_ROOT_PORT_ENABLED,
					BL_FIA_PCIE_ROOT_PORT_4) |
	   BL_FIA_PCIE_ROOT_PORT_CONFIG(BL_ME_FIA_PCIE_ROOT_PORT_STATE,
					BL_ME_FIA_PCIE_ROOT_PORT_DISABLED,
					BL_FIA_PCIE_ROOT_PORT_5) |
	   BL_FIA_PCIE_ROOT_PORT_CONFIG(BL_ME_FIA_PCIE_ROOT_PORT_STATE,
					BL_ME_FIA_PCIE_ROOT_PORT_ENABLED,
					BL_FIA_PCIE_ROOT_PORT_6) |
	   BL_FIA_PCIE_ROOT_PORT_CONFIG(BL_ME_FIA_PCIE_ROOT_PORT_STATE,
					BL_ME_FIA_PCIE_ROOT_PORT_DISABLED,
					BL_FIA_PCIE_ROOT_PORT_7) |
	   BL_FIA_PCIE_ROOT_PORT_CONFIG(
		   BL_ME_FIA_PCIE_ROOT_PORT_LINK_WIDTH,
		   BL_ME_FIA_PCIE_ROOT_PORT_LINK_WIDTH_BICTRL,
		   BL_FIA_PCIE_ROOT_PORT_0) |
	   BL_FIA_PCIE_ROOT_PORT_CONFIG(
		   BL_ME_FIA_PCIE_ROOT_PORT_LINK_WIDTH,
		   BL_ME_FIA_PCIE_ROOT_PORT_LINK_WIDTH_BICTRL,
		   BL_FIA_PCIE_ROOT_PORT_1) |
	   BL_FIA_PCIE_ROOT_PORT_CONFIG(
		   BL_ME_FIA_PCIE_ROOT_PORT_LINK_WIDTH,
		   BL_ME_FIA_PCIE_ROOT_PORT_LINK_WIDTH_BICTRL,
		   BL_FIA_PCIE_ROOT_PORT_2) |
	   BL_FIA_PCIE_ROOT_PORT_CONFIG(
		   BL_ME_FIA_PCIE_ROOT_PORT_LINK_WIDTH,
		   BL_ME_FIA_PCIE_ROOT_PORT_LINK_WIDTH_BICTRL,
		   BL_FIA_PCIE_ROOT_PORT_3) |
	   BL_FIA_PCIE_ROOT_PORT_CONFIG(BL_ME_FIA_PCIE_ROOT_PORT_LINK_WIDTH,
					BL_ME_FIA_PCIE_ROOT_PORT_LINK_X1,
					BL_FIA_PCIE_ROOT_PORT_4) |
	   BL_FIA_PCIE_ROOT_PORT_CONFIG(
		   BL_ME_FIA_PCIE_ROOT_PORT_LINK_WIDTH,
		   BL_ME_FIA_PCIE_ROOT_PORT_LINK_WIDTH_BICTRL,
		   BL_FIA_PCIE_ROOT_PORT_5) |
	   BL_FIA_PCIE_ROOT_PORT_CONFIG(BL_ME_FIA_PCIE_ROOT_PORT_LINK_WIDTH,
					BL_ME_FIA_PCIE_ROOT_PORT_LINK_X1,
					BL_FIA_PCIE_ROOT_PORT_6) |
	   BL_FIA_PCIE_ROOT_PORT_CONFIG(
		   BL_ME_FIA_PCIE_ROOT_PORT_LINK_WIDTH,
		   BL_ME_FIA_PCIE_ROOT_PORT_LINK_WIDTH_BICTRL,
		   BL_FIA_PCIE_ROOT_PORT_7)} } }
};
#endif
#endif
/* _MAINBOARD_HSIO_H */
