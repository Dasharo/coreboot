#!/bin/bash

function usage {
    echo -e "Usage:"
    echo -e "$0 CMD"
    echo -e "Available CMDs:"
    echo -e "\tfw2b - build Protectli FW2B coreboot image"
    echo -e "\tfw4b - build Protectli FW4B coreboot image"
    echo -e "\tfw6  - build Protectli FW6  coreboot image"
    echo -e "\tfw6d - build Protectli FW6D coreboot image"
    exit 1
}

if [ $# -le 0 ]; then
    usage
fi

function buildImage {

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
		-w /home/coreboot/coreboot coreboot/coreboot-sdk:1.52 \
		/bin/bash -c "make distclean"

	cp configs/config.protectli_$1 .config

	echo "Building coreboot for Protectli $1"

	docker run --rm -it -v $PWD:/home/coreboot/coreboot \
		-v $HOME/.ssh:/home/coreboot/.ssh \
		-w /home/coreboot/coreboot coreboot/coreboot-sdk:1.52 \
		/bin/bash -c "make olddefconfig && make && \
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

	cp build/coreboot.rom protectli_$1.rom
	echo "Result binary placed in $PWD/protectli_$1.rom"
	sha256sum protectli_$1.rom > protectli_$1.rom.sha256
}

function buildFW6Image {

	if [ ! -d 3rdparty/blobs/mainboard ]; then
		git submodule update --init --checkout
	fi

	if [ ! -d 3rdparty/blobs/mainboard/protectli/vault_kbl/fw6d ]; then
		wget https://cloud.3mdeb.com/index.php/s/FzF5fjqieEyQX4e/download -O protectli_blobs.zip
		unzip protectli_blobs.zip -d 3rdparty/blobs/mainboard
		rm protectli_blobs.zip
	fi

	docker run --rm -it -v $PWD:/home/coreboot/coreboot \
		-v $HOME/.ssh:/home/coreboot/.ssh \
		-w /home/coreboot/coreboot coreboot/coreboot-sdk:1.52 \
		/bin/bash -c "make distclean"

	cp configs/config.protectli_$1 .config

	echo "Building coreboot for Protectli $1"

	docker run --rm -it -v $PWD:/home/coreboot/coreboot \
		-v $HOME/.ssh:/home/coreboot/.ssh \
		-w /home/coreboot/coreboot coreboot/coreboot-sdk:1.52 \
		/bin/bash -c "make olddefconfig && make && \
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

	cp build/coreboot.rom protectli_$1.rom
	echo "Result binary placed in $PWD/protectli_$1.rom"
	sha256sum protectli_$1.rom > protectli_$1.rom.sha256
}

function buildFW6DImage {

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
		-w /home/coreboot/coreboot coreboot/coreboot-sdk:1.52 \
		/bin/bash -c "make distclean"

	cp configs/config.protectli_$1 .config

	echo "Building coreboot for Protectli $1"

	docker run --rm -it -v $PWD:/home/coreboot/coreboot \
		-v $HOME/.ssh:/home/coreboot/.ssh \
		-w /home/coreboot/coreboot coreboot/coreboot-sdk:1.52 \
		/bin/bash -c "make olddefconfig && make && \
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
		echo \"pci8086,1533.rom pci8086,1539.rom\" > build/links.txt && \
		echo \"pci8086,157b.rom pci8086,1539.rom\" >> build/links.txt && \
		./build/cbfstool build/coreboot.rom add -f build/links.txt -n links -t raw && \
		./build/cbfstool build/coreboot.rom print"

	cp build/coreboot.rom protectli_$1.rom
	echo "Result binary placed in $PWD/protectli_$1.rom"
	sha256sum protectli_$1.rom > protectli_$1.rom.sha256
}

CMD="$1"

case "$CMD" in
    "fw2b")
        buildImage "fw2b"
        ;;
    "fw4b")
        buildImage "fw4b"
        ;;
    "fw6")
        buildFW6Image "fw6"
        ;;
    "fw6d")
        buildFW6DImage "fw6d"
        ;;
    *)
        echo "Invalid command: \"$CMD\""
        ;;
esac
