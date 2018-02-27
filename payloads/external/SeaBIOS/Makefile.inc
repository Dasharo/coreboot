SEABIOSGIT-$(CONFIG_SEABIOS_GENERIC)=http://review.coreboot.org/p/seabios.git
SEABIOSGIT-$(CONFIG_SEABIOS_ELTAN)=https://github.com/pcengines/seabios.git

ifeq ($(CONFIG_SEABIOS_GENERIC),y)
#
# Using the generic seabios
#
TAG-$(CONFIG_SEABIOS_MASTER)=origin/master
TAG-$(CONFIG_SEABIOS_STABLE)=33fbe13a3e2a01e0ba1087a8feed801a0451db21
endif

ifeq ($(CONFIG_SEABIOS_ELTAN),y)
#
# Using the eltan seabios
#
TAG-$(CONFIG_SEABIOS_MASTER)=origin/apu2-support
TAG-$(CONFIG_SEABIOS_STABLE)=$(CONFIG_ELTAN_SEABIOS_TAG)
endif

unexport KCONFIG_AUTOHEADER
unexport KCONFIG_AUTOCONFIG
unexport KCONFIG_DEPENDENCIES
unexport KCONFIG_SPLITCONFIG
unexport KCONFIG_TRISTATE
unexport KCONFIG_NEGATIVES

all: build

seabios:
ifneq ($(wildcard seabios/.*),)
	echo "Found seabios no need to clone"
else
	echo "    Cloning SeaBIOS $(SEABIOSGIT-y)"
	git clone $(SEABIOSGIT-y) seabios
endif

fetch: seabios
ifneq ($(wildcard seabios/.*),)
	echo "Found seabios no need to fetch"
else
	cd seabios; git show $(TAG-y) >/dev/null 2>&1 ;	if [ $$? -ne 0 ]; \
	then echo "    Fetching new commits from the SeaBIOS git repo"; git fetch; fi
endif

checkout: fetch
ifneq ($(wildcard seabios/.*),)
	echo "Found seabios no need to check it out"
else
	echo "    Checking out SeaBIOS revision $(TAG-y)"
	cd seabios; git checkout master; git branch -D coreboot 2>/dev/null; git checkout -b coreboot $(TAG-y)
endif

config: checkout
	echo "    CONFIG     SeaBIOS $(TAG-y)"
	echo "CONFIG_COREBOOT=y" > seabios/.config
ifeq ($(CONFIG_CONSOLE_SERIAL),y)
	echo "CONFIG_DEBUG_SERIAL=y" >> seabios/.config
	echo "CONFIG_DEBUG_SERIAL_PORT=$(CONFIG_TTYS0_BASE)" >> seabios/.config
else
	echo "# CONFIG_DEBUG_SERIAL is not set" >> seabios/.config
endif
ifneq ($(CONFIG_SEABIOS_MALLOC_UPPERMEMORY),y)
	echo "# CONFIG_MALLOC_UPPERMEMORY is not set" >> seabios/.config
endif
ifneq ($(CONFIG_SEABIOS_THREAD_OPTIONROMS),y)
	echo "# CONFIG_THREAD_OPTIONROMS is not set" >> seabios/.config
endif
ifeq ($(CONFIG_SEABIOS_VGA_COREBOOT),y)
	echo "CONFIG_VGA_COREBOOT=y" >> seabios/.config
	echo "CONFIG_BUILD_VGABIOS=y" >> seabios/.config
endif

	echo "CONFIG_DEBUG_LEVEL=-1" >> seabios/.config

#
# Project specific configuration for optimized SeaBIOS
#
	echo "# CONFIG_BOOTSPLASH is not set" >> seabios/.config
	echo "# CONFIG_MEGASAS is not set" >> seabios/.config
	echo "# CONFIG_FLOPPY is not set" >> seabios/.config
	echo "# CONFIG_PS2PORT is not set" >> seabios/.config
	echo "# CONFIG_USB_UHCI is not set" >> seabios/.config
	echo "# CONFIG_USB_OHCI is not set" >> seabios/.config
	echo "# CONFIG_LPT is not set" >> seabios/.config
#
# Enable UDMA to speed up booting
#
	echo "CONFIG_ATA_DMA=y" >> seabios/.config
	echo "CONFIG_ATA_PIO32=y" >> seabios/.config

	# This shows how to force a previously set .config option *off*
	#echo "# CONFIG_SMBIOS is not set" >> seabios/.config

	$(MAKE) -C seabios olddefconfig OUT=out/

build: config
	echo "    MAKE       SeaBIOS $(TAG-y)"
	$(MAKE) -C seabios OUT=out/

clean:
	test -d seabios && $(MAKE) -C seabios clean OUT=out/ || exit 0

distclean:
	rm -rf seabios

.PHONY: checkout config build clean distclean clone fetch
