## SPDX-License-Identifier: GPL-2.0-only

ramstage-$(CONFIG_AP_IN_SIPI_WAIT) += lapic_cpu_stop.c

bootblock-$(CONFIG_UDELAY_LAPIC) += apic_timer.c
romstage-$(CONFIG_UDELAY_LAPIC) += apic_timer.c
ramstage-$(CONFIG_UDELAY_LAPIC) += apic_timer.c
postcar-$(CONFIG_UDELAY_LAPIC) += apic_timer.c
# Use TSC for SMM, should be safer than LAPIC after OS takes over
smm-$(CONFIG_UDELAY_LAPIC) += ../tsc/delay_tsc.c

all_x86-y += boot_cpu.c
all_x86-y += lapic.c
