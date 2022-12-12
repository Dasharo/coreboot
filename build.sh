
#!/bin/bash

function usage {
    echo -e "Usage:"
    echo -e "$0 CMD"
    echo -e "Available CMDs:"
    echo -e "\tvp2420 - build Protectli VP2410 coreboot image"
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
		-w /home/coreboot/coreboot coreboot/coreboot-sdk:b0d87f753c \
		/bin/bash -c "make distclean"

	cp configs/config.protectli_$1 .config

	echo "Building coreboot for Protectli $1"

	docker run --rm -it -v $PWD:/home/coreboot/coreboot \
		-v $HOME/.ssh:/home/coreboot/.ssh \
		-w /home/coreboot/coreboot coreboot/coreboot-sdk:b0d87f753c \
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



function buildVP2420Image {

	if [ ! -d 3rdparty/blobs/mainboard ]; then
		git submodule update --init --checkout
	fi

	if [ ! -d 3rdparty/blobs/mainboard/protectli ]; then
		wget https://cloud.3mdeb.com/index.php/s/FzF5fjqieEyQX4e/download -O protectli_blobs.zip
		unzip protectli_blobs.zip -d 3rdparty/blobs/mainboard
		rm protectli_blobs.zip
	fi

	if [ ! -d 3rdparty/fsp/ElkhartLakeFspBinPkg ]; then
		if [ ! -f ElkhartLakeFspBinPkg.zip ]; then
			echo "Elkhartlake FSP package missing!"
			exit 1
		fi
		unzip ElkhartLakeFspBinPkg.zip -d 3rdparty/fsp/
		rm ElkhartLakeFspBinPkg.zip
	fi

	version=$(cat .coreboot-version)

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

	cp build/coreboot.rom protectli_vp2420_v$version.rom
	if [ $? -eq 0 ]; then
		echo "Result binary placed in $PWD/protectli_vp2420_v$version.rom" 
		sha256sum protectli_vp2420_v$version.rom > protectli_vp2420_v$version.rom.sha256
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
