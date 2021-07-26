#!/bin/bash

function usage {
    echo -e "Usage:"
    echo -e "$0 CMD"
    echo -e "Available CMDs:"
    echo -e "\tfw2b - build Protectli FW2B coreboot image"
    echo -e "\tfw4b - build Protectli FW4B coreboot image"
    echo -e "\tfw6  - build Protectli FW6  coreboot image"
    echo -e "\tcode - build .zip package with source code"
    exit 1
}

if [ $# -le 0 ]; then
    usage
fi

PROTECTLI_BLOBS_URL="https://cloud.3mdeb.com/index.php/s/QajZKSnYieQHBqN/download"
COREBOOT_SDK_VERSION="1.52"
SEABIOS_URL="git@gitlab.com:3mdeb/protectli/seabios.git"
COREBOOT_URL="git@gitlab.com:3mdeb/protectli/coreboot.git"
# Do not run distclean by default, so the submodules etc. are not removed when
# using the zip with source code.
# Can ovveride from envirnment by setting DISTCLEAN=true
DISTCLEAN="${DISTCLEAN:-false}"

function dockerShellCmd {
	local _cmd="$*"

	docker run --rm -it -v $PWD:/home/coreboot/coreboot \
		-v $HOME/.ssh:/home/coreboot/.ssh \
		-w /home/coreboot/coreboot coreboot/coreboot-sdk:$COREBOOT_SDK_VERSION \
		/bin/bash -c "$_cmd"
}

function buildImage {
	local _platform="$1"

	if [ -d coreboot ]; then
	# we are probably using zip with source code and we need to enter
	# coreboot directory first
		cd coreboot
        else
	# we are probably in the coreboot repistory already
		if [ ! -d 3rdparty/blobs/mainboard ]; then
			git submodule update --init --checkout
		fi
	fi

	if [ ! -d 3rdparty/blobs/mainboard/protectli ]; then
		wget $PROTECTLI_BLOBS_URL -O protectli_blobs.zip
		unzip protectli_blobs.zip -d 3rdparty/blobs/mainboard
		rm protectli_blobs.zip
	fi

	if [ "$DISTCLEAN" = "true" ]; then
		dockerShellCmd "make distclean"
	fi

	cp configs/config.protectli_${_platform} .config

	echo "Building coreboot for Protectli $_platform"

	# Use different setup for fw6 than for fw2 and fw4
	if [ "$_platform" = "fw6" ]; then
		dockerShellCmd "make olddefconfig && make -j $(nproc) && \
			./build/cbfstool build/coreboot.rom add-int -i 0x85 -n etc/boot-menu-key && \
			./build/cbfstool build/coreboot.rom add-int -i 1000 -n etc/usb-time-sigatt && \
			./build/cbfstool build/coreboot.rom add-int -i 6000 -n etc/boot-menu-wait && \
			echo \"Press F11 key for boot menu\" > build/message.txt &&
			./build/cbfstool build/coreboot.rom add -f build/message.txt -n etc/boot-menu-message -t raw && \
			echo \"/pci@i0cf8/*@17/drive@0/disk@0\" > build/bootorder.txt && \
			echo \"/pci@i0cf8/usb@14/usb-*@1\" >> build/bootorder.txt && \
			echo \"/pci@i0cf8/usb@14/usb-*@2\" >> build/bootorder.txt && \
			echo \"/pci@i0cf8/usb@14/usb-*@3\" >> build/bootorder.txt && \
			echo \"/pci@i0cf8/usb@14/usb-*@4\" >> build/bootorder.txt && \
			echo \"/pci@i0cf8/*@17/drive@1/disk@0\" >> build/bootorder.txt && \
			./build/cbfstool build/coreboot.rom add -f build/bootorder.txt -n bootorder -t raw && \
			echo \"pci8086,5906.rom pci8086,5916.rom\" > build/links.txt && \
			echo \"pci8086,1533.rom pci8086,150c.rom\" >> build/links.txt && \
			echo \"pci8086,157b.rom pci8086,150c.rom\" >> build/links.txt && \
			./build/cbfstool build/coreboot.rom add -f build/links.txt -n links -t raw && \
			./build/cbfstool build/coreboot.rom print"
	else
		dockerShellCmd "make olddefconfig && make -j $(nproc) && \
			./build/cbfstool build/coreboot.rom add-int -i 0x85 -n etc/boot-menu-key && \
			./build/cbfstool build/coreboot.rom add-int -i 6000 -n etc/usb-time-sigatt && \
			./build/cbfstool build/coreboot.rom add-int -i 4000 -n etc/boot-menu-wait && \
			echo \"Press F11 key for boot menu\" > build/message.txt &&
			./build/cbfstool build/coreboot.rom add -f build/message.txt -n etc/boot-menu-message -t raw && \
			echo \"/pci@i0cf8/*@13/drive@0/disk@0\" > build/bootorder.txt && \
			echo \"/pci@i0cf8/usb@14/usb-*@1\" >> build/bootorder.txt && \
			echo \"/pci@i0cf8/usb@14/usb-*@2\" >> build/bootorder.txt && \
			echo \"/pci@i0cf8/usb@14/hub@3/usb-*@1\" >> build/bootorder.txt && \
			echo \"/pci@i0cf8/usb@14/hub@3/usb-*@2\" >> build/bootorder.txt && \
			echo \"/pci@i0cf8/usb@14/hub@3/usb-*@3\" >> build/bootorder.txt && \
			echo \"/pci@i0cf8/usb@14/hub@3/usb-*@4\" >> build/bootorder.txt && \
			./build/cbfstool build/coreboot.rom add -f build/bootorder.txt -n bootorder -t raw && \
			./build/cbfstool build/coreboot.rom print"
	fi

	local _version=$(cat configs/config.protectli_${_platform} | grep CONFIG_LOCALVERSION | cut -d '=' -f 2 | tr -d '"')
	local _rom_file=protectli_${_platform}_${_version}.rom
	local _sha256_file=${_rom_file}.SHA256

	cp build/coreboot.rom $_rom_file
	echo "Result binary placed in $PWD/$_rom_file"
	sha256sum $_rom_file > $_sha256_file
}

function buildCodeZip {
	local _version=$(git describe --tags)
	mkdir code_release
	if pushd code_release; then
		git clone --depth=1 --branch=$_version $COREBOOT_URL coreboot
		cd coreboot
		git submodule update --init --checkout
		git clone $SEABIOS_URL payloads/external/SeaBIOS/seabios
		cd ../
		cp coreboot/build.sh .
		zip -r -q -9 Protectli_coreboot_${_version}.zip build.sh coreboot
	popd
	fi
}

CMD="$1"

case "$CMD" in
    "fw2b"|"fw4b"|"fw6")
        buildImage "$CMD"
	;;
    "code")
        buildCodeZip "$CMD"
        ;;
    *)
        echo "Invalid command: \"$CMD\""
        ;;
esac
