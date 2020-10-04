/* SPDX-License-Identifier: GPL-2.0-only */

#ifndef __AMDBLOCKS_ACPI_H__
#define __AMDBLOCKS_ACPI_H__

#include <types.h>
#include <soc/nvs.h>

/* ACPI MMIO registers 0xfed80800 */
#define MMIO_ACPI_PM1_STS		0x00
#define MMIO_ACPI_PM1_EN		0x02
#define MMIO_ACPI_PM1_CNT_BLK		0x04
	  /* sleep types defined in arch/x86/include/acpi/acpi.h */
#define   ACPI_PM1_CNT_SCIEN		BIT(0)
#define MMIO_ACPI_PM_TMR_BLK		0x08
#define MMIO_ACPI_CPU_CONTROL		0x0c
#define MMIO_ACPI_GPE0_STS		0x14
#define MMIO_ACPI_GPE0_EN		0x18

/* Structure to maintain standard ACPI register state for reporting purposes. */
struct acpi_pm_gpe_state {
	uint16_t pm1_sts;
	uint16_t pm1_en;
	uint32_t gpe0_sts;
	uint32_t gpe0_en;
	uint16_t previous_sx_state;
	uint16_t aligning_field;
};

/* Fill object with the ACPI PM and GPE state. */
void acpi_fill_pm_gpe_state(struct acpi_pm_gpe_state *state);
/* Save events to eventlog log and also print information on console. */
void acpi_pm_gpe_add_events_print_events(const struct acpi_pm_gpe_state *state);
/* Clear PM and GPE status registers. */
void acpi_clear_pm_gpe_status(void);
/* Fill GNVS object from PM GPE object. */
void acpi_fill_gnvs(struct global_nvs *gnvs, const struct acpi_pm_gpe_state *state);

/*
 * If a system reset is about to be requested, modify the PM1 register so it
 * will never be misinterpreted as an S3 resume.
 */
void set_pm1cnt_s5(void);
void acpi_enable_sci(void);
void acpi_disable_sci(void);

#endif /* __AMDBLOCKS_ACPI_H__ */
