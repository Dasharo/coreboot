## SPDX-License-Identifier: GPL-2.0-only

# force the shell to bash - the edksetup.sh script doesn't work with dash
export SHELL := env bash

project_name=Tianocore
project_dir=$(CURDIR)/tianocore
project_git_repo=https://github.com/mrchromebox/edk2
project_git_branch=coreboot_fb
upstream_git_repo=https://github.com/tianocore/edk2

BUILD_STR = -a IA32 -a X64 -t COREBOOT
ifeq ($(CONFIG_TIANOCORE_COREBOOTPAYLOAD),y)
BUILD_STR += -p CorebootPayloadPkg/CorebootPayloadPkgIa32X64.dsc
else
BUILD_STR += -p UefiPayloadPkg/UefiPayloadPkg.dsc
endif
BUILD_STR += -D BOOTLOADER=COREBOOT -q

#
# EDK II has the following build options relevant to coreboot:
#
#
# OPTION			= DEFAULT_VALUE
#
# ABOVE_4G_MEMORY		= TRUE
ifneq ($(CONFIG_TIANOCORE_ABOVE_4G_MEMORY),y)
BUILD_STR += -D ABOVE_4G_MEMORY=FALSE
endif

export EDK_TOOLS_PATH=$(project_dir)/BaseTools
ifeq ($(CONFIG_TIANOCORE_DEBUG),y)
BUILD_TYPE=DEBUG
else
BUILD_TYPE=RELEASE
endif
# BOOT_MANAGER_ESCAPE		= FALSE
ifeq ($(CONFIG_TIANOCORE_BOOT_MANAGER_ESCAPE),y)
BUILD_STR += -D BOOT_MANAGER_ESCAPE=TRUE
endif

ifeq ($(CONFIG_TIANOCORE_ENABLE_IPXE),y)
build_flavor+= -D NETWORK_IPXE=TRUE
endif

ifeq ($(CONFIG_TIANOCORE_SERIAL),y)
build_flavor+= -D SERIAL_TERMINAL=TRUE
endif

ifeq ($(CONFIG_TIANOCORE_SECURE_BOOT),y)
build_flavor+= -D SECURE_BOOT_ENABLE=TRUE
endif

ifeq ($(CONFIG_TIANOCORE_SATA_PASSWORD),y)
build_flavor+= -D SATA_PASSWORD_ENABLE=TRUE
endif

ifeq ($(CONFIG_TIANOCORE_OPAL_PASSWORD),y)
build_flavor+= -D OPAL_PASSWORD_ENABLE=TRUE
endif

ifeq ($(CONFIG_TIANOCORE_TPM_ENABLE),y)
build_flavor+= -D TPM_ENABLE=TRUE
endif

MENU_KEY=-D BOOT_MENU_KEY=$(CONFIG_TIANOCORE_BOOT_MENU_KEY)
SETUP_KEY=-D SETUP_MENU_KEY=$(CONFIG_TIANOCORE_SETUP_MENU_KEY)
TIMEOUT=-D PLATFORM_BOOT_TIMEOUT=$(CONFIG_TIANOCORE_BOOT_TIMEOUT)

ifeq ($(CONFIG_TIANOCORE_TARGET_IA32), y)
ARCH=-a IA32 -p $(bootloader)/$(bootloader)Ia32.dsc
else
ARCH=-a IA32 -a X64 -p $(bootloader)/$(bootloader)Ia32X64.dsc
endif

BUILD_STR=-q $(ARCH) -t COREBOOT -b $(BUILD_TYPE) $(TIMER) $(TIMEOUT) $(MENU_KEY) $(SETUP_KEY) $(build_flavor)

all: clean build

$(project_dir):
	echo "    Cloning $(project_name) from $(CONFIG_TIANOCORE_REPOSITORY)"
	git clone $(CONFIG_TIANOCORE_REPOSITORY) $(project_dir); \
		cd $(project_dir);

update: $(project_dir)
	if [ ! -d "$(project_dir)" ]; then \
		git clone $(CONFIG_TIANOCORE_REPOSITORY) $(project_dir); \
	fi
	cd  $(project_dir); \
		echo "    Fetching new commits from $(CONFIG_TIANOCORE_REPOSITORY)"; \
		git fetch origin 2>/dev/null; \
		if ! git rev-parse --verify -q $(CONFIG_TIANOCORE_TAG_OR_REV) >/dev/null; then \
			echo "    $(CONFIG_TIANOCORE_TAG_OR_REV) is not a valid git reference"; \
			exit 1; \
		fi; \
		if git status --ignore-submodules=dirty | grep -qv clean; then \
			echo "    Checking out $(project_name) revision $(CONFIG_TIANOCORE_TAG_OR_REV)"; \
			git checkout --detach $(CONFIG_TIANOCORE_TAG_OR_REV) -f; \
		else \
			echo "    Working directory not clean; will not overwrite"; \
		fi; \
		git submodule update --init --checkout

checktools:
	echo "Checking uuid-dev..."
	echo "#include <uuid/uuid.h>" > libtest.c
	echo "int main(int argc, char **argv) { (void) argc; (void) argv; return 0; }" >> libtest.c
	$(HOSTCC) $(HOSTCCFLAGS) libtest.c -o libtest >/dev/null 2>&1 && echo " found uuid-dev." || \
		( echo " Not found."; echo "ERROR: please_install uuid-dev (libuuid-devel)"; exit 1 )
	rm -rf libtest.c libtest
	echo "Checking nasm..."
	type nasm > /dev/null 2>&1 && echo " found nasm." || \
		( echo " Not found."; echo "Error: Please install nasm."; exit 1 )

build: update checktools
	unset CC; $(MAKE) -C $(project_dir)/BaseTools 2>&1
	echo " build $(project_name) $(CONFIG_TIANOCORE_TAG_OR_REV)"
	if [ -n "$(CONFIG_TIANOCORE_BOOTSPLASH_FILE)" ]; then \
		echo "    Copying custom bootsplash image"; \
		case "$(CONFIG_TIANOCORE_BOOTSPLASH_FILE)" in \
			/*) convert $(CONFIG_TIANOCORE_BOOTSPLASH_FILE) \
				BMP3:$(project_dir)/MdeModulePkg/Logo/Logo.bmp;; \
			*) convert $(top)/$(CONFIG_TIANOCORE_BOOTSPLASH_FILE) \
				BMP3:$(project_dir)/MdeModulePkg/Logo/Logo.bmp;; \
		esac \
	fi; \
	cd $(project_dir); \
		export EDK_TOOLS_PATH=$(project_dir)/BaseTools; \
		export WORKSPACE=$(project_dir); \
		. ./edksetup.sh BaseTools; \
		grep -q "COREBOOT" $(project_dir)/Conf/tools_def.txt; \
		if [ $$? -ne 0 ]; then \
			cat ../tools_def.txt >> $(project_dir)/Conf/tools_def.txt; \
		fi; \
		build $(BUILD_STR); \
		mkdir -p $(project_dir)/../output
		mv $(project_dir)/Build/$(bootloader)*/*/FV/UEFIPAYLOAD.fd $(project_dir)/../output/UEFIPAYLOAD.fd; \
		git checkout MdeModulePkg/Logo/Logo.bmp > /dev/null 2>&1 || true

clean:
	test -d $(project_dir) && (cd $(project_dir); rm -rf Build; rm -f Conf/tools_def.txt) || exit 0

distclean:
	rm -rf */

.PHONY: all update checktools config build clean distclean
