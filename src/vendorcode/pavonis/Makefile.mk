## SPDX-License-Identifier: GPL-2.0-only
## 3MDEB CONFIDENTIAL

ifeq ($(CONFIG_PAVONIS_LINUXBOOT_APOB_SYNC),y)

pavonis-dir := $(obj)/pavonis
pavonis-ipcc-rs-src := 3rdparty/ipcc-rs
# Cargo home + target dir live under $(obj) by default; override from the
# environment to keep a persistent crate cache across distcleans.
PAVONIS_CARGO_DIR ?= $(pavonis-dir)/cargo

# preboot.sh: the LinuxBoot uinit that syncs the APOB with the SP before
# kexec. APOB address/size come straight from the SoC Kconfig so the script
# can never drift from the coreboot configuration.
$(pavonis-dir)/preboot.sh: $(src)/vendorcode/pavonis/preboot.sh.in $(DOTCONFIG)
	printf "    GEN        $@\n"
	if [ -z "$(CONFIG_PSP_APOB_DRAM_ADDRESS)" ] || [ -z "$(CONFIG_PSP_APOB_DRAM_SIZE)" ]; then \
		echo "pavonis: CONFIG_PSP_APOB_DRAM_{ADDRESS,SIZE} not set for this SoC" >&2; \
		false; \
	fi
	mkdir -p $(dir $@)
	sed -e "s|@APOB_ADDR@|$(CONFIG_PSP_APOB_DRAM_ADDRESS)|g" \
	    -e "s|@APOB_MAX@|$(CONFIG_PSP_APOB_DRAM_SIZE)|g" \
	    -e "s|@TTY_GLOB@|$(call strip_quotes,$(CONFIG_PAVONIS_PREBOOT_TTY_GLOB))|g" \
	    $< > $@
	chmod +x $@

# faux-ipcc sources come from the 3rdparty/ipcc-rs submodule. The repo is
# private and marked update=none, so populate it on the host (where the ssh
# credentials live) before entering the build container:
#   git submodule update --init --checkout 3rdparty/ipcc-rs
$(pavonis-ipcc-rs-src)/Cargo.toml:
	echo "pavonis: $(pavonis-ipcc-rs-src) submodule is not populated." >&2
	echo "pavonis: run: git submodule update --init --checkout $(pavonis-ipcc-rs-src)" >&2
	false

.PHONY: pavonis-force
pavonis-force:

# faux-ipcc as a fully static binary for the initramfs. The recipe always
# runs and lets cargo decide what to rebuild (incremental no-op builds are
# cheap); the binary is only copied on change so the initramfs is not
# needlessly regenerated.
$(pavonis-dir)/faux-ipcc: $(pavonis-ipcc-rs-src)/Cargo.toml pavonis-force
	printf "    CARGO      faux-ipcc\n"
	command -v cargo >/dev/null 2>&1 || { \
		echo "pavonis: cargo not found in the build environment." >&2; \
		echo "pavonis: faux-ipcc needs Rust with the x86_64-unknown-linux-musl" >&2; \
		echo "pavonis: target (rustup target add x86_64-unknown-linux-musl)." >&2; \
		false; \
	}
	mkdir -p $(PAVONIS_CARGO_DIR) $(dir $@)
	cd $(pavonis-ipcc-rs-src) && \
		CARGO_HOME=$(abspath $(PAVONIS_CARGO_DIR))/home \
		CARGO_TARGET_DIR=$(abspath $(PAVONIS_CARGO_DIR))/target \
		cargo build --locked --release \
			--target x86_64-unknown-linux-musl --bin faux-ipcc
	cmp -s $(PAVONIS_CARGO_DIR)/target/x86_64-unknown-linux-musl/release/faux-ipcc $@ || \
		cp $(PAVONIS_CARGO_DIR)/target/x86_64-unknown-linux-musl/release/faux-ipcc $@

.PHONY: pavonis-faux-ipcc
pavonis-faux-ipcc: $(pavonis-dir)/faux-ipcc

# Inject both artifacts into the u-root cpio. The list is handed to the
# LinuxBoot sub-make (payloads/external/Makefile.mk) which forwards it to
# targets/u-root.mk; entries are absolute src:dest pairs.
#
# preboot.sh lands at bin/preboot, NOT bin/uinit: the u-root fork's console
# multiplexer (cmds/exp/uinit -> /bbin/uinit) is the autostart on this
# platform and would shadow a /bin/uinit anyway (init runs /bbin/uinit
# first and it never exits). Instead conmux chains into us via its
# -bootcmd flag, wired up through uroot.uinitargs=-bootcmd=/bin/preboot
# on the kernel command line.
LINUXBOOT_UROOT_EXTRA_FILES += $(abspath $(pavonis-dir)/faux-ipcc):bin/faux-ipcc
LINUXBOOT_UROOT_EXTRA_FILES += $(abspath $(pavonis-dir)/preboot.sh):bin/preboot

# Without the conmux hook on the kernel command line nothing ever runs
# /bin/preboot; catch that drift at build time.
ifeq ($(findstring -bootcmd=/bin/preboot,$(CONFIG_LINUX_COMMAND_LINE)),)
$(error pavonis: CONFIG_LINUX_COMMAND_LINE lacks uroot.uinitargs=-bootcmd=/bin/preboot; the APOB sync preboot will never be executed)
endif

# Make sure both exist before the LinuxBoot payload (and its initramfs)
# is built.
payloads/external/LinuxBoot/build/Image payloads/external/LinuxBoot/build/initramfs: \
	$(pavonis-dir)/faux-ipcc $(pavonis-dir)/preboot.sh

endif # CONFIG_PAVONIS_LINUXBOOT_APOB_SYNC
