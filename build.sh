#!/bin/bash

docker run --rm -it -u 1000 -v $PWD:/home/coreboot/coreboot \
        -w /home/coreboot/coreboot coreboot/coreboot-sdk:2021-09-23_b0d87f753c \
        /bin/bash -c "make distclean"

git reset --hard HEAD
git clean -df

git fetch https://review.coreboot.org/coreboot refs/changes/78/63578/1 && \
        git format-patch -20 --stdout FETCH_HEAD |git apply

cp configs/config.msi_ms7d25 .config

echo "CONFIG_USE_ADLS_IOT_FSP=y" >> .config
echo "CONFIG_TIANOCORE_BOOTSPLASH_FILE=\"bootsplash.bmp\""  >> .config
echo "CONFIG_POWER_STATE_OFF_AFTER_FAILURE=y"  >> .config

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
        /bin/bash -c "BUILD_TIMELESS=1 make olddefconfig && make"
