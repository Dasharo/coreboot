## SPDX-License-Identifier: GPL-2.0-only

build_dir=$(CURDIR)/build
skiboot_dir=$(CURDIR)/skiboot
skiboot_git_repo=$(CONFIG_SKIBOOT_GIT_REPO)
skiboot_revision=$(CONFIG_SKIBOOT_REVISION)
skiboot_elf=$(build_dir)/skiboot.elf
skiboot_cross=$(or $(CROSS),powerpc64-linux-gnu-)
config=config.its
device_tree_source=$(top)/$(CONFIG_SKIBOOT_DEVICE_TREE)
device_tree_blob=$(build_dir)/fdt.bin
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
	$(MAKE) -C $(skiboot_dir) -j`nproc` CROSS=powerpc64-linux-gnu-

$(skiboot_dir):
	git clone $(skiboot_git_repo) $(skiboot_dir)
	git -C $(skiboot_dir) checkout $(skiboot_revision)

$(build_dir):
	mkdir -p $(build_dir)

distclean: clean
	rm -rf $(skiboot_dir)

clean:
	# Redefine RM because it's used like `$(RM) non-existent-file`
	# Also ignore useless messages about removing test files
	[ ! -d $(skiboot_dir) ] || $(MAKE) -C $(skiboot_dir) RM="rm -rf" clean > /dev/null
	rm -rf $(build_dir)
