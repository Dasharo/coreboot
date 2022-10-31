## SPDX-License-Identifier: GPL-2.0-only

ifeq ($(CONFIG_TPM)$(CONFIG_SPI_TPM),yy)
romstage-y += tis.c tpm.c
ramstage-y += tis.c tpm.c
verstage-y += tis.c tpm.c
endif
