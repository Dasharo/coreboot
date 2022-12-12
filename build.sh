#!/bin/bash

function usage {
    echo -e "Usage:"
    echo -e "$0 CMD"
    echo -e "Available CMDs:"
    echo -e "\tvp2420 - build Protectli VP2420 coreboot image"
    exit 1
}

if [ $# -le 0 ]; then
    usage
fi

function buildVP2420Image {

	if [ ! -d 3rdparty/blobs/mainboard ]; then
		git submodule update --init --checkout
	fi

	if [ ! -d 3rdparty/blobs/mainboard/protectli ]; then
		wget https://cloud.3mdeb.com/index.php/s/FzF5fjqieEyQX4e/download -O protectli_blobs.zip
		unzip protectli_blobs.zip -d 3rdparty/blobs/mainboard
		rm protectli_blobs.zip
	fi

	docker run --rm -it -v $PWD:/home/coreboot/coreboot \
		-v $HOME/.ssh:/home/coreboot/.ssh \
		-w /home/coreboot/coreboot coreboot/coreboot-sdk:2021-09-23_b0d87f753c \
		/bin/bash -c "make distclean"

	cp configs/config.protectli_$1 .config

	echo "Building coreboot for Protectli $1"

	docker run --rm -it -v $PWD:/home/coreboot/coreboot \
		-v $HOME/.ssh:/home/coreboot/.ssh \
		-w /home/coreboot/coreboot coreboot/coreboot-sdk:2021-09-23_b0d87f753c \
		/bin/bash -c "make olddefconfig && make"

	cp build/coreboot.rom protectli_vp2420.rom
	if [ $? -eq 0 ]; then
		echo "Result binary placed in $PWD/protectli_vp2420.rom" 
		sha256sum protectli_vp2420.rom > protectli_vp2420.rom.sha256
	else
		echo "Build failed!"
		exit 1
	fi
}



CMD="$1"

case "$CMD" in
    "vp2420")
        buildVP2420Image "vp2420"
        ;;
    *)
        echo "Invalid command: \"$CMD\""
        ;;
esac
