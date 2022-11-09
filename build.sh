#!/usr/bin/env bash

function usage {
    echo -e "Usage:"
    echo -e "$0 CMD"
    echo -e "Available CMDs:"
    echo -e "\tddr4  - build Dasharo image compatible with MSI PRO Z690-A (WIFI) DDR4"
    echo -e "\tddr5 - build Dasharo image compatible with MSI PRO Z690-A (WIFI)"
    exit 1
}

if [ $# -le 0 ]; then
    usage
fi

function extract_lan_rom()
{
	if which UEFIExtract &> /dev/null; then
		echo "Downloading and extracting original firmware"
		wget https://download.msi.com/bos_exe/mb/7D25v13.zip 2> /dev/null && \
			unzip 7D25v13.zip > /dev/null
		if [[ -f 7D25v13/E7D25IMS.130 ]]; then
			echo "Extracting LAN ROM from vendor firmware"
			UEFIExtract 7D25v13/E7D25IMS.130 DEB917C0-C56A-4860-A05B-BF2F22EBB717 \
				-o ./lanrom -m body > /dev/null && \
			cp lanrom/body_1.bin LanRom.efi && \
			rm -rf lanrom 7D25v13 7D25v13.zip && \
			echo "LAN ROM extracted successfully" && \
			return 0
		fi
		echo "Failed to extract LAN ROM. Network boot will not work."
		return 1
	else
		echo "UEFIExtract not found, but it's required to extract LAN ROM!"
		echo "Install it from: https://github.com/LongSoft/UEFITool/releases/download/A59/UEFIExtract_NE_A59_linux_x86_64.zip"
		echo "Failed to extract LAN ROM. Network boot will not work."
		return 1
	fi
}

function build_image {

	version=$(git describe --abbrev=1 --tags --always --dirty)
	version=${version//msi_ms7d25_/}

	docker run --rm -it -u $UID -v $PWD:/home/coreboot/coreboot \
		-v $HOME/.ssh:/home/coreboot/.ssh \
		-w /home/coreboot/coreboot coreboot/coreboot-sdk:2021-09-23_b0d87f753c \
		/bin/bash -c "make distclean"

	cp configs/config.msi_ms7d25$1 .config

	extract_lan_rom

	if [ $? -eq 0 ]; then
		echo "CONFIG_EDK2_LAN_ROM_DRIVER=\"LanRom.efi\"" >> .config
	fi

	echo "Building Dasharo compatible with MSI PRO Z690-A (WIFI) $2(version $version)"

	docker run --rm -it -u $UID -v $PWD:/home/coreboot/coreboot \
		-v $HOME/.ssh:/home/coreboot/.ssh \
		-w /home/coreboot/coreboot coreboot/coreboot-sdk:2021-09-23_b0d87f753c \
		/bin/bash -c "make olddefconfig && make -j$(nproc)"

	cp build/coreboot.rom msi_ms7d25_${version}$1.rom
	if [ $? -eq 0 ]; then
		echo "Result binary placed in $PWD/msi_ms7d25_${version}$1.rom" 
		sha256sum msi_ms7d25_${version}$1.rom > msi_ms7d25_${version}$1.rom.sha256
	else
		echo "Build failed!"
		exit 1
	fi
}

CMD="$1"

case "$CMD" in
    "ddr4")
        build_image "_ddr4" "DDR4 "
        ;;
    "ddr5")
        build_image "" ""
        ;;
    *)
        echo "Invalid command: \"$CMD\""
        ;;
esac