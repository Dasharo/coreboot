## SPDX-License-Identifier: GPL-2.0-only

ramstage-y += gpio.c
ramstage-y += irqroute.c
ramstage-y += w25q64.c

#SPD_SOURCES  = micron_1GiB_1066MHz_dimm_MT41K256M16HA-125 # 0b0000
#SPD_SOURCES += micron_2GiB_1066MHz_dimm_MT41K256M16HA-125 # 0b0001
#SPD_SOURCES += micron_1GiB_1333MHz_dimm_MT41K256M16HA-125 # 0b0010
#SPD_SOURCES += micron_2GiB_1333MHz_dimm_MT41K256M16HA-125 # 0b0011

SPD_SOURCES  = micron_1GiB_dimm_MT41K256M16HA-125 # 0b0000
SPD_SOURCES += micron_2GiB_dimm_MT41K256M16HA-125 # 0b0001
