romstage-y += ddr_common.c
ramstage-y += ddr_common.c

romstage-$(CONFIG_USE_DDR4) += ddr4.c
ramstage-$(CONFIG_USE_DDR4) += ddr4.c

romstage-$(CONFIG_USE_DDR3) += ddr3.c
ramstage-$(CONFIG_USE_DDR3) += ddr3.c

romstage-$(CONFIG_USE_DDR2) += ddr2.c
ramstage-$(CONFIG_USE_DDR2) += ddr2.c
