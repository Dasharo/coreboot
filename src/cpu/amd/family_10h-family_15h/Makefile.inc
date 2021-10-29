ifeq ($(CONFIG_CPU_AMD_MODEL_10XXX),y)

romstage-y += defaults.c
romstage-y += init_cpus.c
romstage-y += fidvid.c

romstage-y += ram_calc.c
ramstage-y += ram_calc.c

romstage-y += update_microcode.c
ramstage-y += update_microcode.c

endif
