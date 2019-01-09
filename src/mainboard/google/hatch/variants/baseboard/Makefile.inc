## This file is part of the coreboot project.
##
## Copyright (C) 2018 Intel Corporation.
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

SPD_SOURCES = samsung_dimm_K4A4G165WE-BCRC-4GB	# 0b000
SPD_SOURCES += empty_ddr4	# 0b001
SPD_SOURCES += empty_ddr4	# 0b010
SPD_SOURCES += empty_ddr4	# 0b011
SPD_SOURCES += empty_ddr4	# 0b100
SPD_SOURCES += hynix_dimm_H5AN8G6NCJR-VKC-8GB	# 0b101

bootblock-y += gpio.c

romstage-y += gpio.c
romstage-y += memory.c

ramstage-y += gpio.c

verstage-y += gpio.c
