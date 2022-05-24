#!/usr/bin/env bash

function extract_microcode()
{
        if which UEFIExtract &> /dev/null; then
                echo "Downloading and extracting vendor firmware"
                wget https://download.msi.com/bos_exe/mb/7D25v13.zip 2> /dev/null && \
                         unzip 7D25v13.zip > /dev/null
                if [[ -f 7D25v13/E7D25IMS.130 ]]; then
                        echo "Extracting microcode from vendor firmware"
                        UEFIExtract 7D25v13/E7D25IMS.130 17088572-377F-44EF-8F4E-B09FFF46A070 \
                                -o 7D25v13/microcode -m file -t FF > /dev/null && \
                                dd if=7D25v13/microcode/file.ffs of=adl-s_microcode.bin \
                                        skip=24 iflag=skip_bytes 2> /dev/null && \
                                echo "Microcode extracted successfully" && \
                                rm -rf 7D25v13* && return 0

                fi
                echo "Failed to extract microcode"
                rm -rf 7D25v13*
        else
                echo "UEFIExtract not found, but it's required to extract microcode!"
                echo "Install it from: https://github.com/LongSoft/UEFITool/releases/download/A59/UEFIExtract_NE_A59_linux_x86_64.zip"
        fi
}

docker run --rm -it -u 1000 -v $PWD:/home/coreboot/coreboot \
        -w /home/coreboot/coreboot coreboot/coreboot-sdk:2021-09-23_b0d87f753c \
        /bin/bash -c "make distclean"

extract_microcode

git reset --hard HEAD
git clean -df

git fetch https://review.coreboot.org/coreboot refs/changes/40/64640/1 && \
        git format-patch -50 --stdout FETCH_HEAD | git apply

cp configs/config.msi_ms7d25 .config

if [ "$SERIAL_CONSOLE" != "yes" ]; then
echo "# CONFIG_CONSOLE_SERIAL is not set" >> .config
else
echo "CONFIG_TIANOCORE_SERIAL=y" >> .config
fi

echo "CONFIG_USE_ADLS_IOT_FSP=y" >> .config
echo "CONFIG_TIANOCORE_BOOTSPLASH_FILE=\"bootsplash.bmp\""  >> .config
echo "CONFIG_TIANOCORE_SECURE_BOOT=y"  >> .config
echo "CONFIG_TIANOCORE_BOOT_MENU_KEY=0x0015"  >> .config
echo "CONFIG_TIANOCORE_SETUP_MENU_KEY=0x0008"  >> .config
sed -i 's/origin\/dasharo/0c94299bdad2a85ca0e44bd491e996adc2cc606f/g' .config

echo "CONFIG_VBOOT=y" >> .config

git submodule update --init --checkout

if [ ! -d 3rdparty/fsp/AlderLakeFspBinPkg ]; then
        echo "Alder Lake FSP missing! You must obtain it first."
        exit 1
fi

if [ ! -f adl-s_microcode.bin ]; then
        echo "Platform microcode blob missing!"
        echo "Extract it from vendor fw first and save as adl-s_microcode.bin"
        exit 1
fi

echo "Building Dasharo compatible with MSI PRO Z690-A WiFi DDR4"

docker run --rm -it -u 1000 -v $PWD:/home/coreboot/coreboot \
        -w /home/coreboot/coreboot coreboot/coreboot-sdk:2021-09-23_b0d87f753c \
        /bin/bash -c "make olddefconfig && make"
