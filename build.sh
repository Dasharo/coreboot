#!/bin/bash

function usage {
    echo -e "Usage:"
    echo -e "$0 CMD"
    echo -e "Available CMDs:"
    echo -e "\tpt201 - build Protectli PT201 coreboot image"
    echo -e "\tpt401 - build Protectli PT401 coreboot image"
    echo -e "\tpt601 - build Protectli PT601 coreboot image"
    exit 1
}

if [ $# -le 0 ]; then
    usage
fi

function buildImage {

	if [ ! -d 3rdparty/blobs/mainboard ]; then
		git submodule update --init --checkout
	fi

	docker run --rm -t -u $UID -v $PWD:/home/coreboot/coreboot \
		-v $HOME/.ssh:/home/coreboot/.ssh \
		-w /home/coreboot/coreboot coreboot/coreboot-sdk:2021-09-23_b0d87f753c \
		/bin/bash -c "make distclean"

	cp configs/config.protectli_vault_jsl_$1 .config

	echo "Building coreboot for Protectli $1"

	docker run --rm -t -u $UID -v $PWD:/home/coreboot/coreboot \
		-v $HOME/.ssh:/home/coreboot/.ssh \
		-w /home/coreboot/coreboot coreboot/coreboot-sdk:2021-09-23_b0d87f753c \
		/bin/bash -c "make olddefconfig && make"

	cp build/coreboot.rom protectli_$1.rom
	echo "Result binary placed in $PWD/protectli_$1.rom"
	sha256sum protectli_$1.rom > protectli_$1.rom.sha256
}

CMD="$1"

case "$CMD" in
    "pt201")
        buildImage "pt201"
        ;;
    "pt401")
        buildImage "pt401"
        ;;
    "pt601")
        buildImage "pt601"
        ;;
    *)
        echo "Invalid command: \"$CMD\""
        ;;
esac
