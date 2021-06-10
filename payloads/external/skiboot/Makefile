## SPDX-License-Identifier: GPL-2.0-only

build_dir=$(CURDIR)/build
config=config.its
device_tree_source=$(top)/$(CONFIG_SKIBOOT_DEVICE_TREE)
device_tree_blob=$(build_dir)/fdt.bin
skiboot_dir=$(CURDIR)/talos-skiboot
skiboot_git_repo=https://git.raptorcs.com/git/talos-skiboot
skiboot=$(skiboot_dir)/skiboot.lid
uimage=$(build_dir)/skiboot.uimage

unexport $(COREBOOT_EXPORTS)

.PHONY: all clean distclean

all: $(uimage)

$(uimage): $(build_dir) $(skiboot) $(device_tree_blob)
	mkimage -f $(config) $(uimage)

$(device_tree_blob): $(build_dir) $(device_tree_source)
	dtc -I dts -O dtb -o $(device_tree_blob) $(device_tree_source)

$(skiboot): $(skiboot_dir)
	$(MAKE) -C $(skiboot_dir) -j`nproc` CROSS=powerpc64-linux-gnu- DEBUG=1

$(skiboot_dir):
	git clone $(skiboot_git_repo) $(skiboot_dir)
	cd $(skiboot_dir) && git checkout $(CONFIG_SKIBOOT_REVISION)

$(build_dir):
	mkdir -p $(build_dir)

distclean:
	rm -rf $(skiboot_dir)
	rm -rf $(build_dir)

clean:
	if [ -d $(skiboot_dir) ]; then \
		$(MAKE) -C $(skiboot_dir) CROSS=powerpc64-linux-gnu- RM="rm -f" clean > /dev/null; \
	fi
	rm -rf $(build_dir)
