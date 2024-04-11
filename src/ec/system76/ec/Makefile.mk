# SPDX-License-Identifier: GPL-2.0-only

ifeq ($(CONFIG_EC_SYSTEM76_EC),y)

all-y += system76_ec.c
all-y += buttons.c
smm-$(CONFIG_DEBUG_SMI) += system76_ec.c

cbfs-files-$(CONFIG_EC_SYSTEM76_EC_UPDATE) += ec.rom
ec.rom-file :=$(call strip_quotes,$(CONFIG_EC_SYSTEM76_EC_UPDATE_FILE))
ec.rom-compression := $(CBFS_COMPRESS_FLAG)
ec.rom-type := raw

endif
