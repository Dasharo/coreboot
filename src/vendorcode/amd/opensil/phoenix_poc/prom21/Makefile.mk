## SPDX-License-Identifier: GPL-2.0-only

ramstage-$(CONFIG_DRIVERS_AMD_PROMONTORY21) += chip_to_opensil.c

$(obj)/ramstage/vendorcode/amd/opensil/phoenix_poc/promontory21/chip_to_opensil.o: CFLAGS_ramstage += -D_MSC_EXTENSIONS=0 -DHAS_STRING_H=1 -Wno-unknown-pragmas
