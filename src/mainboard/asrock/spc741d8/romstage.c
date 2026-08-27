/* SPDX-License-Identifier: GPL-2.0-only */

#include <console/console.h>
#include <drivers/ocp/ewl/ocp_ewl.h>
#include <option.h>
#include <soc/romstage.h>
#include <soc/ddr.h>
#include <stdio.h>
#include <types.h>
#include <defs_cxl.h>
#include <defs_iio.h>
#include <mainboard_iio.h>

void mainboard_ewl_check(void)
{
	get_ewl();
}

#define IIO_PORTS_PER_IOU	8
#define IIO_FIRST_IOU_PORT(iou)	(1 + (iou) * IIO_PORTS_PER_IOU)

static bool iio_bifurcation_valid(unsigned int bifurcation)
{
	switch (bifurcation) {
	case IIO_BIFURCATE_x4x4x4x4:
	case IIO_BIFURCATE_x4x4xxx8:
	case IIO_BIFURCATE_xxx8x4x4:
	case IIO_BIFURCATE_xxx8xxx8:
	case IIO_BIFURCATE_xxxxxx16:
	case IIO_BIFURCATE_AUTO:
		return true;
	default:
		return false;
	}
}
static uint8_t iio_bifurcation_port_mask(unsigned int bifurcation)
{
	switch (bifurcation) {
	case IIO_BIFURCATE_xxxxxx16:	/* x16      */
		return BIT(0);
	case IIO_BIFURCATE_xxx8xxx8:	/* x8NAx8NA */
		return BIT(0) | BIT(4);
	case IIO_BIFURCATE_xxx8x4x4:	/* x8NAx4x4 */
		return BIT(0) | BIT(4) | BIT(6);
	case IIO_BIFURCATE_x4x4xxx8:	/* x4x4x8NA */
		return BIT(0) | BIT(2) | BIT(4);
	case IIO_BIFURCATE_x4x4x4x4:	/* x4x4x4x4 */
	default:
		return BIT(0) | BIT(2) | BIT(4) | BIT(6);
	}
}

static void mainboard_config_iio_bifurcation(void)
{
	for (unsigned int iou = 0; iou < ARRAY_SIZE(iio_bifur[0]); iou++) {
		char opt_name[sizeof("iio_bifurcation_iou0")];

		snprintf(opt_name, sizeof(opt_name), "iio_bifurcation_iou%u", iou);

		unsigned int bifurcation = get_uint_option(opt_name, iio_bifur[0][iou]);
		if (!iio_bifurcation_valid(bifurcation)) {
			printk(BIOS_WARNING, "IIO: %s has invalid value 0x%x, using 0x%x\n",
			       opt_name, bifurcation, iio_bifur[0][iou]);
			bifurcation = iio_bifur[0][iou];
		}

		iio_bifur[0][iou] = bifurcation;

		const uint8_t port_mask = iio_bifurcation_port_mask(bifurcation);

		for (unsigned int port = 0; port < IIO_PORTS_PER_IOU; port++) {
			const unsigned int index = IIO_FIRST_IOU_PORT(iou) + port;
			UPD_IIO_PCIE_PORT_CONFIG_ENTRY *entry = &iio_pci_port[0][index];

			if (port_mask & BIT(port)) {
				entry->PEXPHIDE = 0;
				entry->SLOTIMP = 1;
				entry->SLOTPSP = index;
			} else {
				entry->PEXPHIDE = 1;
				entry->SLOTIMP = 0;
				entry->SLOTPSP = 0;
			}
		}

		printk(BIOS_DEBUG, "IIO: IOU%u bifurcation 0x%02x, root ports 0x%02x\n",
		       iou, bifurcation, port_mask);
	}
}

static void mainboard_config_iio(FSPM_UPD *mupd)
{
	/* Set socket 0 IIO PCIe PE0,PE1,PE2,PE3 to CXL mode */
	mupd->FspmConfig.IioPcieSubSystemMode0[0] = IIO_MODE_CXL;
	mupd->FspmConfig.IioPcieSubSystemMode1[0] = IIO_MODE_CXL;
	mupd->FspmConfig.IioPcieSubSystemMode2[0] = IIO_MODE_CXL;
	mupd->FspmConfig.IioPcieSubSystemMode3[0] = IIO_MODE_CXL;

	mupd->FspmConfig.DfxCxlHeaderBypass = 0;
	mupd->FspmConfig.DfxCxlSecLvl = CXL_SECURITY_FULLY_TRUSTED;

	mupd->FspmConfig.DelayAfterPCIeLinkTraining = 2000; /* ms */
}

void mainboard_memory_init_params(FSPM_UPD *mupd)
{
	/* Send FSP log message to main serial port */
	mupd->FspmConfig.SerialIoUartDebugEnable = 1;
	mupd->FspmConfig.SerialIoUartDebugIoBase = CONFIG_TTYS0_BASE;

	/* Set Rank Margin Tool to disable. */
	mupd->FspmConfig.EnableRMT = 0x0;
	/* Enable - Portions of memory reference code will be skipped
	 * when possible to increase boot speed on warm boots.
	 * Disable - Disables this feature.
	 * Auto - Sets it to the MRC default setting.
	 */
	mupd->FspmConfig.AttemptFastBoot = 0x1;
	mupd->FspmConfig.AttemptFastBootCold = 0x1;

	/* Set Adv MemTest Option to 0. */
	mupd->FspmConfig.AdvMemTestOptions = 0x0;
	/* Set MRC Promote Warnings to disable.
	   Determines if MRC warnings are promoted to system level. */
	mupd->FspmConfig.promoteMrcWarnings = 0x0;
	/* Set Promote Warnings to disable.
	   Determines if warnings are promoted to system level. */
	mupd->FspmConfig.promoteWarnings = 0x0;
	mainboard_config_iio_bifurcation();
	soc_config_iio(mupd, iio_pci_port, iio_bifur);
	mainboard_config_iio(mupd);
}

bool mainboard_dimm_slot_exists(uint8_t socket, uint8_t channel, uint8_t dimm)
{
	if (socket >= CONFIG_MAX_SOCKET)
		return false;
	// SPC741D8 supports 8 channels with 1 DIMM each
	if (channel >= 8)
		return false;
	if (dimm >= 1)
		return false;

	return true;
}
