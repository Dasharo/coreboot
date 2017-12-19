#
# This file is part of the coreboot project.
#
# Copyright (C) 2012 Advanced Micro Devices, Inc.
# Copyright (C) 2016 Eltan B.V.
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; version 2 of the License.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
#

romstage-y += agesawrapper.c
romstage-y += BiosCallOuts.c
romstage-y += PlatformGnbPcie.c
romstage-y += bios_knobs.c

ramstage-y += agesawrapper.c
ramstage-y += BiosCallOuts.c
ramstage-y += PlatformGnbPcie.c
ramstage-y += bios_knobs.c

# WIV20150202 add ramtest
cbfs-files-y += img/memtest
img/memtest-file := payloads/eltan/memtest86+/memtest501.serial.com1.nospd.eltan-elf
img/memtest-position := 0xcd900
img/memtest-type := payload
# WIV20150202 add ramtest

cbfs-files-y += bootorder_map
bootorder_map-file := bootorder_map
bootorder_map-type := raw

cbfs-files-y += bootorder_def
bootorder_def-file := bootorder_def
bootorder_def-type := raw

# WIV20150205 add ipxe
#cbfs-files-y += genroms/pxe.rom
#genroms/pxe.rom-file := payloads/eltan/ipxe/10ec8168.rom
#genroms/pxe.rom-type := raw
# WIV20150205 add ipxe

# WIV20150126 add boot order
cbfs-files-y += bootorder
bootorder-file := src/mainboard/$(MAINBOARDDIR)/bootorder
bootorder-type := raw
bootorder-align := 0x1000

cbfs-files-y += etc/boot-menu-message
etc/boot-menu-message-file := src/mainboard/$(MAINBOARDDIR)/boot-menu-message
etc/boot-menu-message-type := raw

cbfs-files-y += etc/boot-menu-key
etc/boot-menu-key-file := src/mainboard/$(MAINBOARDDIR)/boot-menu-key
etc/boot-menu-key-type := raw

cbfs-files-y += etc/boot-menu-wait
etc/boot-menu-wait-file := src/mainboard/$(MAINBOARDDIR)/boot-menu-wait
etc/boot-menu-wait-type := raw
# WIV20150126 end add boot order

# add a SeaBIOS sercon-port file
cbfs-files-y += etc/sercon-port
etc/sercon-port-file := src/mainboard/$(MAINBOARDDIR)/sercon-port
etc/sercon-port-type := raw

# WIV20141001 START ADD SPD FROM FILE
## DIMM SPD for on-board memory
SPD_BIN = $(obj)/spd.bin

# Order of names in SPD_SOURCES is important!
SPD_SOURCES  = HYNIX-2G-1333 HYNIX-4G-1333-ECC
#SPD_SOURCES  = HYNIX-2G-1333  HYNIX-4G-1333-NOECC

SPD_DEPS := $(foreach f, $(SPD_SOURCES), src/mainboard/$(MAINBOARDDIR)/$(f).spd.hex)

# Include spd rom data
$(SPD_BIN): $(SPD_DEPS) src/mainboard/$(MAINBOARDDIR)/Makefile.inc
	echo "    create SPD $@"
	for f in $(SPD_DEPS); \
	  do for c in $$(cat $$f | grep -v ^#); \
	    do printf $$(printf '\%o' 0x$$c); \
	  done; \
	done > $@

cbfs-files-y += spd.bin
spd.bin-file := $(SPD_BIN)
spd.bin-type := 0xab
# WIV20141001 END ADD SPD FROM FILE

