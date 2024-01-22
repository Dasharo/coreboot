# SPDX-License-Identifier: GPL-2.0-only

ifeq ($(CONFIG_EC_SYSTEM76_EC),y)

all-y += system76_ec.c
all-y += buttons.c
smm-$(CONFIG_DEBUG_SMI) += system76_ec.c
cbfs-files-y += ec.rom
ec.rom-file := $(obj)/ec.rom
ec.rom-type := raw

endif
