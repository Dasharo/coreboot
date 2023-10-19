#!/usr/bin/env bash

function usage {
    echo -e "Usage:"
    echo -e "$0 CMD"
    echo -e "Available CMDs:"
    echo -e "\tvp46xx - build Protectli VP46xx Dasharo image"
    exit 1
}

if [ $# -le 0 ]; then
    usage
fi

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

	docker run --rm -t -u $UID -v $PWD:/home/coreboot/coreboot \
		-v $HOME/.ssh:/home/coreboot/.ssh \
		-w /home/coreboot/coreboot coreboot/coreboot-sdk:2021-09-23_b0d87f753c \
		/bin/bash -c "make distclean"

	cp configs/config.protectli_cml_vp46xx .config

	echo "Building Dasharo for Protectli VP46XX (version $version)"

	docker run --rm -t -u $UID -v $PWD:/home/coreboot/coreboot \
		-v $HOME/.ssh:/home/coreboot/.ssh \
		-w /home/coreboot/coreboot coreboot/coreboot-sdk:2021-09-23_b0d87f753c \
		/bin/bash -c "make olddefconfig && make -j$(nproc)"

	cp build/coreboot.rom protectli_vault_cml_${version}_vp46xx.rom
	if [ $? -eq 0 ]; then
		echo "Result binary placed in $PWD/protectli_vault_cml_${version}_vp46xx.rom"
		sha256sum protectli_vault_cml_${version}_vp46xx.rom > protectli_vault_cml_${version}_vp46xx.rom.sha256
	else
		echo "Build failed!"
		exit 1
	fi
}

CMD="$1"

case "$CMD" in
    "vp46xx")
        buildVP46xxImage
        ;;
    *)
        echo "Invalid command: \"$CMD\""
        ;;
esac
