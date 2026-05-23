## SPDX-License-Identifier: GPL-2.0-only

bootblock-y += bootblock.c
bootblock-y += timer.c

romstage-y += ../qemu-i440fx/memmap.c
romstage-y += timer.c

postcar-y += ../qemu-i440fx/memmap.c
postcar-y += ../qemu-i440fx/exit_car.S
postcar-y += timer.c

ramstage-y += ../qemu-i440fx/memmap.c
ramstage-y += ../qemu-i440fx/northbridge.c
ramstage-y += ../qemu-i440fx/rom_media.c
ramstage-y += cpu.c
ramstage-y += timer.c

all-y += ../qemu-i440fx/bootmode.c
all-y += memmap.c

ramstage-$(CONFIG_CHROMEOS) += chromeos.c

smm-y += ../qemu-i440fx/rom_media.c
smm-y += smihandler.c
