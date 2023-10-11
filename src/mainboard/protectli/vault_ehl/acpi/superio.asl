/* SPDX-License-Identifier: GPL-2.0-only */

#undef SUPERIO_DEV
#undef SUPERIO_PNP_BASE
#undef IT8613E_SHOW_UARTA
#undef IT8613E_SHOW_KBC
#undef IT8613E_SHOW_PS2M
#define SUPERIO_DEV		SIO0
#define SUPERIO_PNP_BASE	0x2e
#define IT8613E_SHOW_UARTA	1

#include <superio/ite/it8613e/acpi/superio.asl>
