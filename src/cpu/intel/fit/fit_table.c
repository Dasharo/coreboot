/* SPDX-License-Identifier: GPL-2.0-only */

#include <arch/fit_table.h>

__attribute((used)) static struct fit_entry fit_table[CONFIG_CPU_INTEL_NUM_FIT_ENTRIES + 1] = {
	[0] = {
		.address.bytes = {'_', 'F', 'I', 'T', '_', ' ', ' ', ' '},
		.size_reserved = 1,
		.version = 0x100,
		.type_checksum_valid = FIT_ENTRY_HAS_CHECKSUM | FIT_ENTRY_TYPE_HEADER,
		.checksum = 0x7d,
	}
};
