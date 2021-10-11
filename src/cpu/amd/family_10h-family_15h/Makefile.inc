ifeq ($(CONFIG_CPU_AMD_MODEL_10XXX),y)

romstage-y += ram_calc.c
ramstage-y += ram_calc.c

endif
