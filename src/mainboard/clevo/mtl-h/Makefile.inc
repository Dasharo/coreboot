# SPDX-License-Identifier: GPL-2.0-only

CPPFLAGS_common += -I$(src)/mainboard/$(MAINBOARDDIR)/include

bootblock-y += bootblock.c
bootblock-y += gpio_early.c

romstage-y += romstage.c

ramstage-y += ramstage.c
ramstage-y += hda_verb.c
ramstage-y += gpio.c

ramstage-$(CONFIG_DRIVERS_I2C_TAS5825M) += variants/$(VARIANT_DIR)/tas5825m.c
ramstage-$(CONFIG_HAVE_ACPI_TABLES) += fadt.c

SPD_SOURCES = samsung-P4AAF165WA-BCWDE
