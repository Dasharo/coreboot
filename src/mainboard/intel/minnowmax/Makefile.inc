##
## This file is part of the coreboot project.
##
## Copyright (C) 2013 Google Inc.
##
## This program is free software; you can redistribute it and/or modify
## it under the terms of the GNU General Public License as published by
## the Free Software Foundation; version 2 of the License.
##
## This program is distributed in the hope that it will be useful,
## but WITHOUT ANY WARRANTY; without even the implied warranty of
## MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
## GNU General Public License for more details.
##

ramstage-y += gpio.c
ramstage-y += irqroute.c

#SPD_SOURCES  = micron_1GiB_1066MHz_dimm_MT41K256M16HA-125 # 0b0000
#SPD_SOURCES += micron_2GiB_1066MHz_dimm_MT41K256M16HA-125 # 0b0001
#SPD_SOURCES += micron_1GiB_1333MHz_dimm_MT41K256M16HA-125 # 0b0010
#SPD_SOURCES += micron_2GiB_1333MHz_dimm_MT41K256M16HA-125 # 0b0011

SPD_SOURCES  = micron_1GiB_dimm_MT41K256M16HA-125 # 0b0000
SPD_SOURCES += micron_2GiB_dimm_MT41K256M16HA-125 # 0b0001
