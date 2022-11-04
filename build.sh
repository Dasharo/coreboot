#!/usr/bin/env bash

function usage {
    echo -e "Usage:"
    echo -e "$0 CMD"
    echo -e "Available CMDs:"
    echo -e "\tvp4630_vp4650  - build Protectli VP4630/VP4650 Dasharo image"
    echo -e "\tvp4670 - build Protectli VP4670 Dasharo image"
    exit 1
}

if [ $# -le 0 ]; then
    usage
fi

function extract_lan_rom()
{
	if which UEFIExtract &> /dev/null; then
		echo "Downloading and extracting original firmware"
		wget https://github.com/protectli-root/protectli-firmware-updater/raw/289a8c60881707db934ee707a7e3ddccfa185b94/images/vp4630_YW6L2314.bin 2> /dev/null
		if [[ -f vp4630_YW6L2314.bin ]]; then
			echo "Extracting LAN ROM from vendor firmware"
			UEFIExtract vp4630_YW6L2314.bin AA5618A2-5EEB-434C-9E58-4123C039FE69 \
				-o ./lanrom -m body > /dev/null && \
			cp lanrom/body.bin LanRom.efi && \
			rm -rf lanrom vp4630_YW6L2314.bin && \
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

function buildVP46xxImage {

	if [ ! -d 3rdparty/blobs/mainboard ]; then
		git submodule update --init --checkout
	fi

	if [ ! -d 3rdparty/blobs/mainboard/protectli/vault_cml ]; then
		if [ -f protectli_blobs.zip ]; then
			unzip protectli_blobs.zip -d 3rdparty/blobs/mainboard
		else
			echo "Platform blobs missing! You must obtain them first."
			exit 1
		fi
	fi

	version=$(git describe --abbrev=1 --tags --always --dirty)
	version=${version//protectli_vault_cml_/}

	docker run --rm -it -u $UID -v $PWD:/home/coreboot/coreboot \
		-v $HOME/.ssh:/home/coreboot/.ssh \
		-w /home/coreboot/coreboot coreboot/coreboot-sdk:2021-09-23_b0d87f753c \
		/bin/bash -c "make distclean"

	cp configs/config.protectli_cml_$1 .config

	extract_lan_rom

	if [ $? -eq 0 ]; then
		echo "CONFIG_EDK2_LAN_ROM_DRIVER=\"LanRom.efi\"" >> .config
	fi

	echo "Building Dasharo for Protectli $2 (version $version)"

	docker run --rm -it -u $UID -v $PWD:/home/coreboot/coreboot \
		-v $HOME/.ssh:/home/coreboot/.ssh \
		-w /home/coreboot/coreboot coreboot/coreboot-sdk:2021-09-23_b0d87f753c \
		/bin/bash -c "make olddefconfig && make -j$(nproc)"

	cp build/coreboot.rom protectli_vault_cml_${version}_$1.rom
	if [ $? -eq 0 ]; then
		echo "Result binary placed in $PWD/protectli_vault_cml_${version}_$1.rom" 
		sha256sum protectli_vault_cml_${version}_$1.rom > protectli_vault_cml_${version}_$1.rom.sha256
	else
		echo "Build failed!"
		exit 1
	fi
}

CMD="$1"

case "$CMD" in
    "vp4630_vp4650")
        buildVP46xxImage "vp4630_vp4650" "VP4630/VP4650"
        ;;
    "vp4670")
        buildVP46xxImage "vp4670" "VP4670"
        ;;
    *)
        echo "Invalid command: \"$CMD\""
        ;;
esac
