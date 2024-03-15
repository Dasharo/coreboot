/* SPDX-License-Identifier: GPL-2.0-or-later */

#include <acpi/acpi.h>
#include <console/console.h>
#include <intelblocks/oc_wdt.h>

#define OC_WDT_MIN_VALUE	0
#define OC_WDT_MAX_VALUE	1023
#define OC_WDT_PERIOD_MS	1000

static bool fill_wdat_timeout_entry(acpi_wdat_entry_t *entry)
{
	memset((void *)entry, 0, sizeof(acpi_wdat_entry_t));

	entry->action = ACPI_WDAT_SET_COUNTDOWN;
	entry->instruction = ACPI_WDAT_WRITE_COUNTDOWN | ACPI_WDAT_PRESERVE_REGISTER;
	entry->mask = PCH_OC_WDT_CTL_TOV_MASK;
	entry->register_region.space_id = ACPI_ADDRESS_SPACE_IO;
	entry->register_region.addrl = PCH_OC_WDT_CTL;
	entry->register_region.access_size = ACPI_WDAT_ACCESS_SIZE_WORD;

	return true;
}

static bool fill_wdat_boot_status_entry(acpi_wdat_entry_t *entry, uint8_t action,
					uint8_t instruction, uint32_t value)
{
	memset((void *)entry, 0, sizeof(acpi_wdat_entry_t));

	entry->action = action;
	entry->instruction = instruction;
	entry->value = value;
	entry->mask = PCH_OC_WDT_CTL_SCRATCH_MASK >> PCH_OC_WDT_CTL_SCRATCH_OFFSET;
	entry->register_region.space_id = ACPI_ADDRESS_SPACE_IO;
	entry->register_region.addrl = PCH_OC_WDT_CTL + 2;
	entry->register_region.access_size = ACPI_WDAT_ACCESS_SIZE_BYTE;

	return true;
}

static bool fill_wdat_run_state_entry(acpi_wdat_entry_t *entry, uint8_t action,
				       uint8_t instruction, uint32_t value)
{
	memset((void *)entry, 0, sizeof(acpi_wdat_entry_t));

	entry->action = action;
	entry->instruction = instruction;
	entry->value = value;
	entry->mask = PCH_OC_WDT_CTL_EN;
	entry->register_region.space_id = ACPI_ADDRESS_SPACE_IO;
	entry->register_region.addrl = PCH_OC_WDT_CTL;
	entry->register_region.access_size = ACPI_WDAT_ACCESS_SIZE_WORD;

	return true;
}

static bool fill_wdat_ping_entry(acpi_wdat_entry_t *entry)
{
	memset((void *)entry, 0, sizeof(acpi_wdat_entry_t));

	entry->action = ACPI_WDAT_RESET;
	entry->instruction = ACPI_WDAT_WRITE_VALUE;
	entry->value = PCH_OC_WDT_CTL_RLD >> 24;
	entry->mask = PCH_OC_WDT_CTL_RLD >> 24;
	entry->register_region.space_id = ACPI_ADDRESS_SPACE_IO;
	entry->register_region.addrl = PCH_OC_WDT_CTL + 3;
	entry->register_region.access_size = ACPI_WDAT_ACCESS_SIZE_BYTE;

	return true;
}

unsigned long acpi_soc_fill_wdat(acpi_wdat_t *wdat, unsigned long current)
{
	if (!wdat)
		return current;

	wdat->pci_segment = 0xff;
	wdat->pci_bus = 0xff;
	wdat->pci_device = 0xff;
	wdat->pci_function = 0xff;

	wdat->timer_period = OC_WDT_PERIOD_MS;
	wdat->min_count = OC_WDT_MIN_VALUE;
	wdat->max_count = OC_WDT_MAX_VALUE;
	wdat->flags = ACPI_WDAT_FLAG_ENABLED;
	wdat->entries = 0;

	acpi_wdat_entry_t *entry = (acpi_wdat_entry_t *)current;

	/* Write countdown */
	if (!fill_wdat_timeout_entry(entry))
		goto out_err;

	entry++;

	/* Get boot status */
	if (!fill_wdat_boot_status_entry(entry, ACPI_WDAT_GET_STATUS,
					 ACPI_WDAT_READ_VALUE, 1))
		goto out_err;

	entry++;

	/* Set boot status */
	if (!fill_wdat_boot_status_entry(entry, ACPI_WDAT_SET_STATUS,
					 ACPI_WDAT_WRITE_VALUE,
					 0))
		goto out_err;

	entry++;

	/* Get running status */
	if (!fill_wdat_run_state_entry(entry, ACPI_WDAT_GET_RUNNING_STATE,
				       ACPI_WDAT_READ_VALUE, PCH_OC_WDT_CTL_EN))
		goto out_err;

	entry++;

	/* Start the watchdog */
	if (!fill_wdat_run_state_entry(entry, ACPI_WDAT_SET_RUNNING_STATE,
				       ACPI_WDAT_WRITE_VALUE | ACPI_WDAT_PRESERVE_REGISTER,
				       PCH_OC_WDT_CTL_EN))
		goto out_err;

	entry++;

	/* Get stopped status */
	if (!fill_wdat_run_state_entry(entry, ACPI_WDAT_GET_STOPPED_STATE,
				       ACPI_WDAT_READ_VALUE, 0))
		goto out_err;

	entry++;

	/* Stop the watchdog */
	if (!fill_wdat_run_state_entry(entry, ACPI_WDAT_SET_STOPPED_STATE,
				       ACPI_WDAT_WRITE_VALUE | ACPI_WDAT_PRESERVE_REGISTER,
				       0))
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
