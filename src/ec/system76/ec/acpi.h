#ifndef EC_SYSTEM76_EC_ACPI_H
#define EC_SYSTEM76_EC_ACPI_H

#include <ec/acpi/ec.h>

#define SYSTEM76_EC_REG_LSTE			0x03
#define SYSTEM76_EC_REG_LSTE_LID_STATE	0x01

int system76_ec_get_lid_state(void);

#endif /* EC_SYSTEM76_EC_ACPI_H */