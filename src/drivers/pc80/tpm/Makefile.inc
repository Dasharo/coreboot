## SPDX-License-Identifier: GPL-2.0-only

ifeq ($(CONFIG_MEMORY_MAPPED_TPM),y)
romstage-y += tis.c
ramstage-y += tis.c
verstage-y += tis.c
endif
