#!/bin/bash

set -euo pipefail

usage() {
  echo "${0} CMD"
  echo "Available CMDs:"
  echo -e "\tz690a_ddr4  - build Dasharo image compatible with MSI PRO Z690-A (WIFI) DDR4"
  echo -e "\tz690a_ddr5 - build Dasharo image compatible with MSI PRO Z690-A (WIFI)"
  echo -e "\tz790p_ddr4  - build Dasharo image compatible with MSI PRO Z790-P (WIFI) DDR4"
  echo -e "\tz790p_ddr5 - build Dasharo image compatible with MSI PRO Z790-P (WIFI)"
  echo -e "\tvp46xx- build Dasharo for Protectli VP46xx"
}

SDKVER="2021-09-23_b0d87f753c"

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

function build_msi {
  DEFCONFIG="configs/config.${BOARD}_$1"
  FW_VERSION=$(cat ${DEFCONFIG} | grep CONFIG_LOCALVERSION | cut -d '=' -f 2 | tr -d '"')

  docker run --rm -t -u $UID -v $PWD:/home/coreboot/coreboot \
    -v $HOME/.ssh:/home/coreboot/.ssh \
    -w /home/coreboot/coreboot coreboot/coreboot-sdk:$SDKVER \
    /bin/bash -c "make distclean"

  cp "${DEFCONFIG}" .config

  extract_lan_rom

  if [ $? -eq 0 ]; then
    echo "CONFIG_EDK2_LAN_ROM_DRIVER=\"LanRom.efi\"" >> .config
  fi

  git submodule update --init --checkout

  echo "Building Dasharo compatible with MSI PRO $2(WIFI) (version $FW_VERSION)"

  docker run --rm -t -u $UID -v $PWD:/home/coreboot/coreboot \
    -v $HOME/.ssh:/home/coreboot/.ssh \
    -w /home/coreboot/coreboot coreboot/coreboot-sdk:$SDKVER \
    /bin/bash -c "make olddefconfig && make -j$(nproc)"

  cp build/coreboot.rom ${BOARD}_${FW_VERSION}_$1.rom
  if [ $? -eq 0 ]; then
    echo "Result binary placed in $PWD/${BOARD}_${FW_VERSION}_$1.rom"
    sha256sum ${BOARD}_${FW_VERSION}_$1.rom > ${BOARD}_${FW_VERSION}_$1.rom.sha256
  else
    echo "Build failed!"
    exit 1
  fi
}

function build_vp46xx {
	DEFCONFIG="configs/config.protectli_cml_vp46xx"
	FW_VERSION=$(cat ${DEFCONFIG} | grep CONFIG_LOCALVERSION | cut -d '=' -f 2 | tr -d '"')

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

	docker run --rm -t -u $UID -v $PWD:/home/coreboot/coreboot \
		-v $HOME/.ssh:/home/coreboot/.ssh \
		-w /home/coreboot/coreboot coreboot/coreboot-sdk:2021-09-23_b0d87f753c \
		/bin/bash -c "make distclean"

	cp configs/config.protectli_cml_vp46xx .config

	echo "Building Dasharo for Protectli VP46XX (version $FW_VERSION)"

	docker run --rm -t -u $UID -v $PWD:/home/coreboot/coreboot \
		-v $HOME/.ssh:/home/coreboot/.ssh \
		-w /home/coreboot/coreboot coreboot/coreboot-sdk:$SDKVER \
		/bin/bash -c "make olddefconfig && make -j$(nproc)"

	cp build/coreboot.rom protectli_vault_cml_${FW_VERSION}_vp46xx.rom
	if [ $? -eq 0 ]; then
		echo "Result binary placed in $PWD/protectli_vault_cml_${FW_VERSION}_vp46xx.rom"
		sha256sum protectli_vault_cml_${FW_VERSION}_vp46xx.rom > protectli_vault_cml_${FW_VERSION}_vp46xx.rom.sha256
	else
		echo "Build failed!"
		exit 1
	fi
}

function build_v1x10 {
	DEFCONFIG="configs/config.protectli_vault_jsl_$1"
	FW_VERSION=$(cat ${DEFCONFIG} | grep CONFIG_LOCALVERSION | cut -d '=' -f 2 | tr -d '"')

	if [ ! -d 3rdparty/blobs/mainboard ]; then
		git submodule update --init --checkout
	fi

	if [ ! -d 3rdparty/blobs/mainboard/protectli/vault_jsl ]; then
		if [ -f protectli_blobs.zip ]; then
			unzip protectli_blobs.zip -d 3rdparty/blobs/mainboard
		else
			echo "Platform blobs missing! You must obtain them first."
			exit 1
		fi
	fi

	docker run --rm -t -u $UID -v $PWD:/home/coreboot/coreboot \
		-v $HOME/.ssh:/home/coreboot/.ssh \
		-w /home/coreboot/coreboot coreboot/coreboot-sdk:2021-09-23_b0d87f753c \
		/bin/bash -c "make distclean"

	cp $DEFCONFIG .config

	echo "Building Dasharo for Protectli $1 (version $FW_VERSION)"

	docker run --rm -t -u $UID -v $PWD:/home/coreboot/coreboot \
		-v $HOME/.ssh:/home/coreboot/.ssh \
		-w /home/coreboot/coreboot coreboot/coreboot-sdk:$SDKVER \
		/bin/bash -c "make olddefconfig && make -j$(nproc)"

	cp build/coreboot.rom protectli_$1_${FW_VERSION}.rom
	if [ $? -eq 0 ]; then
		echo "Result binary placed in $PWD/protectli_$1_${FW_VERSION}.rom"
		sha256sum protectli_$1_${FW_VERSION}.rom > protectli_$1_${FW_VERSION}.rom.sha256
	else
		echo "Build failed!"
		exit 1
	fi
}


CMD="$1"

case "$CMD" in
    "ddr4" | "z690a_ddr4")
        BOARD="msi_ms7d25"
        build_msi ddr4 "Z690-A DDR4 "
        ;;
    "ddr5" | "z690a_ddr5")
        BOARD="msi_ms7d25"
        build_msi ddr5 "Z690-A DDR5 "
        ;;
    "z790p_ddr4")
        BOARD="msi_ms7e06"
        build_msi ddr4 "Z790-P DDR4 "
        ;;
    "z790p_ddr5")
        BOARD="msi_ms7e06"
        build_msi ddr5 "Z790-P DDR5 "
        ;;
    "vp46xx")
        build_vp46xx
        ;;
    "v1210" | "v1410" | "v1610")
        build_v1x10 $1
        ;;
    *)
        echo "Invalid command: \"$CMD\""
        usage
        ;;
esac
