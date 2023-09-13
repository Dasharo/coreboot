## SPDX-License-Identifier: GPL-2.0-only

CPPFLAGS_common += -Isrc/vendorcode/dasharo/include/

smm-y += options.c
all-y += options.c

ramstage-y += smbios.c
