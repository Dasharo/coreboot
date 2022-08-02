## SPDX-License-Identifier: GPL-2.0-only

build_dir=$(CURDIR)/build
skiboot_dir=$(CURDIR)/talos-skiboot
skiboot_elf=$(build_dir)/skiboot.elf
skiboot_cross=$(or $(CROSS),powerpc64-linux-gnu-)

unexport $(COREBOOT_EXPORTS)

.PHONY: all clean distclean

all: $(skiboot_elf)

$(skiboot_elf): | $(skiboot_dir) $(build_dir)
	+$(MAKE) -C $(skiboot_dir) CROSS="$(skiboot_cross)"
	cp $(skiboot_dir)/skiboot.elf $@
	# skiboot is always built with debug information due to unconditional -ggdb
	$(skiboot_cross)strip $@

$(skiboot_dir):
	git clone $(CONFIG_SKIBOOT_REPOSITORY) $(skiboot_dir)
	cd $(skiboot_dir) && git checkout $(CONFIG_SKIBOOT_REVISION)

$(build_dir):
	mkdir -p $(build_dir)

distclean:
	rm -rf $(skiboot_dir)
	rm -rf $(build_dir)

clean:
	if [ -d $(skiboot_dir) ]; then \
		$(MAKE) -C $(skiboot_dir) "CROSS=$(skiboot_cross)" RM="rm -f" clean > /dev/null; \
	fi
	rm -rf $(build_dir)
