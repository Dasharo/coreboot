/* SPDX-License-Identifier: GPL-2.0-only */

#include <acpi/acpi.h>
#include <boardid.h>
#include <ec/google/chromeec/ec.h>
#include <drivers/intel/gma/opregion.h>
#include <baseboard/variants.h>
#include <delay.h>
#include <gpio.h>
#include <variant/sku.h>

const char *mainboard_vbt_filename(void)
{
	uint32_t sku_id;

	sku_id = google_chromeec_get_board_sku();

	switch (sku_id) {
	case SKU_9_HDMI:
	case SKU_19_HDMI_TS:
	case SKU_50_HDMI:
	case SKU_52_HDMI_TS:
		return "vbt_garg_hdmi.bin";
	default:
		return "vbt.bin";
	}
}

void variant_smi_sleep(u8 slp_typ)
{
	/* Currently use cases here all target to S5 therefore we do early return
	 * here for saving one transaction to the EC for getting SKU ID. */
	if (slp_typ != ACPI_S5)
		return;

	switch (google_chromeec_get_board_sku()) {
	case SKU_17_LTE:
	case SKU_18_LTE_TS:
		power_off_lte_module();
		return;
	default:
		return;
	}
}
