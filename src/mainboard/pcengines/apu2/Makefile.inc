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
ramstage-y += s1_button.c

cbfs-files-y += bootorder_map
ifeq ($(CONFIG_BOARD_PCENGINES_APU5),y)
bootorder_map-file := bootorder_map_apu5
else
bootorder_map-file := bootorder_map
endif
bootorder_map-type := raw

cbfs-files-y += bootorder_def
ifeq ($(CONFIG_BOARD_PCENGINES_APU5),y)
bootorder_def-file := bootorder_def_apu5
else
bootorder_def-file := bootorder_def
endif
bootorder_def-type := raw

# WIV20150126 add boot order
cbfs-files-y += bootorder
bootorder-file := src/mainboard/$(MAINBOARDDIR)/variants/$(VARIANT_DIR)/bootorder
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

# memtest86+

ifeq ($(CONFIG_CONSOLE_SERIAL)$(CONFIG_DRIVERS_UART_8250IO),yy)
	MEMTEST_SERIAL_OPTIONS=SERIAL_CONSOLE_DEFAULT=1 \
		SERIAL_TTY=$(CONFIG_UART_FOR_CONSOLE) \
		SERIAL_BAUD_RATE=$(CONFIG_TTYS0_BAUD)
endif

memtest: $(DOTCONFIG)
	$(MAKE) -C payloads/external/Memtest86Plus -f Makefile.inc all \
		CC="$(CC_x86_32)" \
		LD="$(LD_x86_32)" \
		OBJCOPY="$(OBJCOPY_x86_32)" \
		AS="$(AS_x86_32)" \
		CONFIG_MEMTEST_REVISION=$(CONFIG_MEMTEST_REVISION) \
		CONFIG_MEMTEST_REVISION_ID=$(CONFIG_MEMTEST_REVISION_ID) \
		CONFIG_MEMTEST_MASTER=$(CONFIG_MEMTEST_MASTER) \
		CONFIG_MEMTEST_STABLE=$(CONFIG_MEMTEST_STABLE) \
		$(MEMTEST_SERIAL_OPTIONS) \
		MFLAGS= MAKEFLAGS=

# iPXE

ifeq ($(CONFIG_CONSOLE_SERIAL)$(CONFIG_DRIVERS_UART_8250IO),yy)
IPXE_UART=COM$(call int-add,$(CONFIG_UART_FOR_CONSOLE) 1)
endif

ifeq ($(CONFIG_PXE_SERIAL_CONSOLE),y)
IPXE_SERIAL_CONSOLE = $(CONFIG_CONSOLE_SERIAL)$(CONFIG_DRIVERS_UART_8250IO)
else
IPXE_SERIAL_CONSOLE = n
endif

ifeq ($(CONFIG_PXE_ROM),y)
PXE_ROM_FILE:=$(CONFIG_PXE_ROM_FILE)
endif
ifeq ($(CONFIG_BUILD_IPXE),y)
PXE_ROM_FILE:=payloads/external/iPXE/ipxe/ipxe.rom
endif

ipxe: $(DOTCONFIG)
	$(MAKE) -C payloads/external/iPXE -f Makefile.inc all \
			CROSS_COMPILE="$(CROSS_COMPILE_$(ARCH-ramstage-y))" \
			PXE_ROM_PCI_ID=$(subst $(comma),,$(CONFIG_PXE_ROM_ID)) \
			CONFIG_IPXE_MASTER=$(CONFIG_IPXE_MASTER) \
			CONFIG_IPXE_STABLE=$(CONFIG_IPXE_STABLE) \
			CONSOLE_SERIAL=$(IPXE_SERIAL_CONSOLE) \
			IPXE_UART=$(IPXE_UART) \
			CONFIG_TTYS0_BAUD=$(CONFIG_TTYS0_BAUD) \
			CONFIG_PXE_CUSTOM_GENERAL_H=$(CONFIG_PXE_CUSTOM_GENERAL_H) \
			CONFIG_PXE_CUSTOM_BOOTMENU_FILE=$(CONFIG_PXE_CUSTOM_BOOTMENU_FILE) \
			CONFIG_PXE_CUSTOM_BUILD_ID=$(CONFIG_PXE_CUSTOM_BUILD_ID)

subdirs-y += variants/$(VARIANT_DIR)
