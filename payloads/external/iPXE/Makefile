## SPDX-License-Identifier: GPL-2.0-only

# 2023.8 - Last commit of August 2023
# When updating, change the name both here and in payloads/external/iPXE/Kconfig

# Temporarily Dasharo fork is used until i225/i226 native support is merged.
STABLE_COMMIT_ID=838611b34e172baee46a10b443e9f1e5f6e6344f

TAG-$(CONFIG_IPXE_MASTER)=origin/master
TAG-$(CONFIG_IPXE_STABLE)=$(STABLE_COMMIT_ID)

project_name=iPXE
project_dir=ipxe
project_git_repo=https://github.com/Dasharo/ipxe.git

unexport KCONFIG_AUTOHEADER
unexport KCONFIG_AUTOCONFIG
unexport KCONFIG_DEPENDENCIES
unexport KCONFIG_SPLITCONFIG
unexport KCONFIG_TRISTATE
unexport KCONFIG_NEGATIVES

ifneq ($(CONFIG_IPXE_CUSTOM_BUILD_ID),)
PXE_MAKE_OPTS := BUILD_ID_CMD="echo 0x$(CONFIG_IPXE_CUSTOM_BUILD_ID)"
endif

ifeq ($(CONFIG_IPXE_BUILD_EFI),y)
IPXE_BUILD_TARGET := bin-x86_64-efi-sb/ipxe.efi
# Workaround problems with unrecognized ipxe/util/elf2efi relocations
PXE_MAKE_OPTS += EXTRA_CFLAGS="$(CFLAGS_x86_64) -fno-pic"
else
IPXE_BUILD_TARGET := bin/$(PXE_ROM_PCI_ID).rom
endif

all: build

$(project_dir):
	echo "    Cloning $(project_name) from Git"
	git clone $(project_git_repo) $(project_dir)

fetch: $(project_dir)
	cd $(project_dir); \
		git show $(TAG-y) >/dev/null 2>&1 ; \
		if [ $$? -ne 0 ] || [ "$(TAG-y)" = "origin/master" ]; then \
			echo "    Fetching new commits from the $(project_name) repo"; \
			git fetch; \
		fi

checkout: fetch
	echo "    Checking out $(project_name) revision $(TAG-y)"
	cd  $(project_dir); \
		git checkout -- src/config/general.h; \
		git checkout master; \
		git branch -D coreboot 2>/dev/null; \
		git checkout -b coreboot $(TAG-y)

config: checkout
ifeq ($(CONSOLE_SERIAL),yy)
	cp "$(project_dir)/src/config/console.h" "$(project_dir)/src/config/console.h.cb"
	cp "$(project_dir)/src/config/serial.h" "$(project_dir)/src/config/serial.h.cb"
	sed -i'' 's|//#define\s*CONSOLE_SERIAL.*|#define CONSOLE_SERIAL|' "$(project_dir)/src/config/console.h"
	sed -i'' 's|#define\s*COMCONSOLE.*|#define COMCONSOLE $(IPXE_UART)|' "$(project_dir)/src/config/serial.h"
	sed -i'' 's|#define\s*COMSPEED.*|#define COMSPEED $(CONFIG_TTYS0_BAUD)|'  "$(project_dir)/src/config/serial.h"
endif
ifneq ($(filter y,$(CONFIG_HAS_SCRIPT) $(CONFIG_IPXE_NO_PROMPT)),)
	cp "$(project_dir)/src/config/general.h" "$(project_dir)/src/config/general.h.cb"
endif
ifeq ($(CONFIG_HAS_SCRIPT),y)
	sed -i'' 's|//#define\s*IMAGE_SCRIPT.*|#define IMAGE_SCRIPT|' "$(project_dir)/src/config/general.h"
endif
ifeq ($(CONFIG_IPXE_NO_PROMPT),y)
	sed -i'' 's|#define\s*BANNER_TIMEOUT.*|#define BANNER_TIMEOUT 0|' "$(project_dir)/src/config/general.h"
endif
ifeq ($(CONFIG_IPXE_HAS_HTTPS),y)
	sed -i'' 's|.*DOWNLOAD_PROTO_HTTPS|#define DOWNLOAD_PROTO_HTTPS|g' "$(project_dir)/src/config/general.h"
endif
ifeq ($(CONFIG_PXE_TRUST_CMD),y)
	sed 's|.*IMAGE_TRUST_CMD|#define IMAGE_TRUST_CMD|g' "$(project_dir)/src/config/general.h" > "$(project_dir)/src/config/general.h.tmp"
	mv "$(project_dir)/src/config/general.h.tmp" "$(project_dir)/src/config/general.h"
endif

build: config $(CONFIG_SCRIPT)
ifeq ($(CONFIG_HAS_SCRIPT),y)
	echo "    MAKE       $(project_name) $(TAG-y) EMBED=$(CONFIG_SCRIPT)"
	$(MAKE) -C $(project_dir)/src $(IPXE_BUILD_TARGET) EMBED=$(CONFIG_SCRIPT) $(PXE_MAKE_OPTS)
else
	echo "    MAKE       $(project_name) $(TAG-y)"
	$(MAKE) -C $(project_dir)/src $(IPXE_BUILD_TARGET) $(PXE_MAKE_OPTS)
endif
	cp $(project_dir)/src/$(IPXE_BUILD_TARGET) $(project_dir)/ipxe.rom
ifeq ($(CONSOLE_SERIAL),yy)
	cp "$(project_dir)/src/config/console.h.cb" "$(project_dir)/src/config/console.h"
	cp "$(project_dir)/src/config/serial.h.cb" "$(project_dir)/src/config/serial.h"
endif
ifneq ($(filter y,$(CONFIG_HAS_SCRIPT) $(CONFIG_IPXE_NO_PROMPT)),)
	cp "$(project_dir)/src/config/general.h.cb" "$(project_dir)/src/config/general.h"
endif

clean:
	test -d $(project_dir) && $(MAKE) -C $(project_dir)/src veryclean || exit 0
	rm -f $(project_dir)/ipxe.rom

distclean:
	rm -rf $(project_dir)

.PHONY: all fetch config build clean distclean
