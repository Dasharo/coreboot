## SPDX-License-Identifier: GPL-2.0-only

ifeq ($(CONFIG_TPM)$(CONFIG_I2C_TPM),yy)

romstage-$(CONFIG_DRIVER_TIS_DEFAULT) += tis.c
ramstage-$(CONFIG_DRIVER_TIS_DEFAULT) += tis.c
verstage-$(CONFIG_DRIVER_TIS_DEFAULT) += tis.c

romstage-$(CONFIG_TPM_ATMEL) += tis_atmel.c
ramstage-$(CONFIG_TPM_ATMEL) += tis_atmel.c
verstage-$(CONFIG_TPM_ATMEL) += tis_atmel.c

ifeq ($(CONFIG_TPM_GOOGLE),y)

romstage-y += cr50.c
ramstage-y += cr50.c
verstage-y += cr50.c
else

romstage-y += tpm.c
ramstage-y += tpm.c
verstage-y += tpm.c

endif

endif

ramstage-$(CONFIG_DRIVER_I2C_TPM_ACPI) += chip.c
