ifeq ($(CONFIG_CPU_AMD_MODEL_10XXX),y)

romstage-y += defaults.c
romstage-y += init_cpus.c
romstage-y += fidvid.c

romstage-y += ram_calc.c
ramstage-y += ram_calc.c

romstage-y += update_microcode.c
ramstage-y += update_microcode.c

bootblock-y += tsc_freq.c
verstage-y += tsc_freq.c
romstage-y += tsc_freq.c
postcar-y += tsc_freq.c
ramstage-y += tsc_freq.c
smm-y += tsc_freq.c

bootblock-y += monotonic_timer.c
verstage-y += monotonic_timer.c
romstage-y += monotonic_timer.c
postcar-y += monotonic_timer.c
ramstage-y += monotonic_timer.c
smm-y += monotonic_timer.c

ramstage-y += powernow_acpi.c
ramstage-y += processor_name.c
ramstage-y += model_10xxx_init.c

endif
