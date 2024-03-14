/* SPDX-License-Identifier: GPL-2.0-or-later */

#include <acpi/acpi.h>
#include <console/console.h>
#include <intelblocks/tco.h>
#include <soc/intel/common/tco.h>

static bool fill_wdat_timeout_entry(acpi_wdat_entry_t *entry)
{
	uint16_t tcobase = tco_get_bar();

	if (tcobase == 0)
		return false;

	memset((void *)entry, 0, sizeof(acpi_wdat_entry_t));

	entry->action = ACPI_WDAT_SET_COUNTDOWN;
	entry->instruction = ACPI_WDAT_WRITE_COUNTDOWN | ACPI_WDAT_PRESERVE_REGISTER;
	entry->mask = TCO_TMR_MASK;
	entry->register_region.space_id = ACPI_ADDRESS_SPACE_IO;
	entry->register_region.addrl = tcobase + TCO_TMR;
	entry->register_region.access_size = ACPI_WDAT_ACCESS_SIZE_WORD;

	return true;
}

static bool fill_wdat_boot_status_entry(acpi_wdat_entry_t *entry, uint8_t action,
					uint8_t instruction, uint32_t value)
{
	uint16_t tcobase = tco_get_bar();

	if (tcobase == 0)
		return false;

	memset((void *)entry, 0, sizeof(acpi_wdat_entry_t));

	entry->action = action;
	entry->instruction = instruction;
	entry->value = value;
	entry->mask = TCO2_STS_SECOND_TO;
	entry->register_region.space_id = ACPI_ADDRESS_SPACE_IO;
	entry->register_region.addrl = tcobase + TCO_MESSAGE1;
	entry->register_region.access_size = ACPI_WDAT_ACCESS_SIZE_BYTE;

	return true;
}

static bool fill_wdat_run_state_entry(acpi_wdat_entry_t *entry, uint8_t action,
				       uint8_t instruction, uint32_t value)
{
	uint16_t tcobase = tco_get_bar();

	if (tcobase == 0)
		return false;

	memset((void *)entry, 0, sizeof(acpi_wdat_entry_t));

	entry->action = action;
	entry->instruction = instruction;
	entry->value = value;
	entry->mask = TCO1_TMR_HLT;
	entry->register_region.space_id = ACPI_ADDRESS_SPACE_IO;
	entry->register_region.addrl = tcobase + TCO1_CNT;
	entry->register_region.access_size = ACPI_WDAT_ACCESS_SIZE_WORD;

	return true;
}

static bool fill_wdat_ping_entry(acpi_wdat_entry_t *entry)
{
	uint16_t tcobase = tco_get_bar();

	if (tcobase == 0)
		return false;

	memset((void *)entry, 0, sizeof(acpi_wdat_entry_t));

	entry->action = ACPI_WDAT_RESET;
	entry->instruction = ACPI_WDAT_WRITE_VALUE;
	entry->value = 0x01;
	entry->mask = 0x01;
	entry->register_region.space_id = ACPI_ADDRESS_SPACE_IO;
	entry->register_region.addrl = tcobase + TCO_RLD;
	entry->register_region.access_size = ACPI_WDAT_ACCESS_SIZE_WORD;

	return true;
}

unsigned long acpi_soc_fill_wdat(acpi_wdat_t *wdat, unsigned long current)
{
	if (!wdat)
		return current;

	uint16_t tcobase = tco_get_bar();

	if (tcobase == 0)
		goto out_err;

	wdat->pci_segment = 0xff;
	wdat->pci_bus = 0xff;
	wdat->pci_device = 0xff;
	wdat->pci_function = 0xff;

	wdat->timer_period = tco_get_timer_period();
	wdat->min_count = tco_get_timer_min_value();
	wdat->max_count = tco_get_timer_max_value();
	wdat->flags = ACPI_WDAT_FLAG_ENABLED;
	wdat->entries = 0;

	acpi_wdat_entry_t *entry = (acpi_wdat_entry_t *)current;

	/* Write countdown */
	if (!fill_wdat_timeout_entry(entry))
		goto out_err;

	entry++;

	/* Get boot status */
	if (!fill_wdat_boot_status_entry(entry, ACPI_WDAT_GET_STATUS,
					 ACPI_WDAT_READ_VALUE, TCO2_STS_SECOND_TO))
		goto out_err;

	entry++;

	/* Set boot status */
	if (!fill_wdat_boot_status_entry(entry, ACPI_WDAT_SET_STATUS,
					 ACPI_WDAT_WRITE_VALUE | ACPI_WDAT_PRESERVE_REGISTER,
					 0))
		goto out_err;

	entry++;

	/* Get running status */
	if (!fill_wdat_run_state_entry(entry, ACPI_WDAT_GET_RUNNING_STATE,
				       ACPI_WDAT_READ_VALUE, 0))
		goto out_err;

	entry++;

	/* Start the watchdog */
	if (!fill_wdat_run_state_entry(entry, ACPI_WDAT_SET_RUNNING_STATE,
				       ACPI_WDAT_WRITE_VALUE | ACPI_WDAT_PRESERVE_REGISTER,
				       0))
		goto out_err;

	entry++;

	/* Get stopped status */
	if (!fill_wdat_run_state_entry(entry, ACPI_WDAT_GET_STOPPED_STATE,
				       ACPI_WDAT_READ_VALUE, TCO1_TMR_HLT))
		goto out_err;

	entry++;

	/* Stop the watchdog */
	if (!fill_wdat_run_state_entry(entry, ACPI_WDAT_SET_STOPPED_STATE,
				       ACPI_WDAT_WRITE_VALUE | ACPI_WDAT_PRESERVE_REGISTER,
				       TCO1_TMR_HLT))
		goto out_err;

	entry++;

	/* Ping */
	if (!fill_wdat_ping_entry(entry))
		goto out_err;

	entry++;

	wdat->entries = ((unsigned long)entry - current) / sizeof(acpi_wdat_entry_t);

	return (unsigned long)entry;

out_err:
	wdat->flags = ACPI_WDAT_FLAG_DISABLED;
	printk(BIOS_ERR, "Fail to populate WDAT ACPI Table");

	return current;
}
