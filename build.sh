#!/bin/bash

set -euo pipefail

usage() {
  echo "${0} CMD"
  echo "Available CMDs:"
  echo -e "\tz690a_ddr4  - build Dasharo image compatible with MSI PRO Z690-A (WIFI) DDR4"
  echo -e "\tz690a_ddr5 - build Dasharo image compatible with MSI PRO Z690-A (WIFI)"
  echo -e "\tz790p_ddr4  - build Dasharo image compatible with MSI PRO Z790-P (WIFI) DDR4"
  echo -e "\tz790p_ddr5 - build Dasharo image compatible with MSI PRO Z790-P (WIFI)"
  echo -e "\tvp66xx- build Dasharo for Protectli VP66xx"
  echo -e "\tvp46xx- build Dasharo for Protectli VP46xx"
  echo -e "\tvp2420- build Dasharo for Protectli VP2420"
  echo -e "\tvp2410- build Dasharo for Protectli VP2410"
  echo -e "\tV1210- build Dasharo for Protectli V1210"
  echo -e "\tV1410- build Dasharo for Protectli V1410"
  echo -e "\tV1610- build Dasharo for Protectli V1610"
}

SDKVER="2021-09-23_b0d87f753c"

function build_msi {
  DEFCONFIG="configs/config.${BOARD}_$1"
  FW_VERSION=$(cat ${DEFCONFIG} | grep CONFIG_LOCALVERSION | cut -d '=' -f 2 | tr -d '"')

  docker run --rm -t -u $UID -v $PWD:/home/coreboot/coreboot \
    -v $HOME/.ssh:/home/coreboot/.ssh \
    -w /home/coreboot/coreboot coreboot/coreboot-sdk:$SDKVER \
    /bin/bash -c "make distclean"

  cp "${DEFCONFIG}" .config

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

function build_protectli_vault {
  DEFCONFIG="configs/config.protectli_${BOARD}"
  FW_VERSION=$(cat ${DEFCONFIG} | grep CONFIG_LOCALVERSION | cut -d '=' -f 2 | tr -d '"')

  if [ ! -d 3rdparty/dasharo-blobs/protectli ]; then
    git submodule update --init --checkout
  fi

  docker run --rm -t -u $UID -v $PWD:/home/coreboot/coreboot \
    -v $HOME/.ssh:/home/coreboot/.ssh \
    -w /home/coreboot/coreboot coreboot/coreboot-sdk:2021-09-23_b0d87f753c \
    /bin/bash -c "make distclean"

  cp $DEFCONFIG .config

  echo "Building Dasharo for Protectli $BOARD (version $FW_VERSION)"

  docker run --rm -t -u $UID -v $PWD:/home/coreboot/coreboot \
    -v $HOME/.ssh:/home/coreboot/.ssh \
    -w /home/coreboot/coreboot coreboot/coreboot-sdk:$SDKVER \
    /bin/bash -c "make olddefconfig && make -j$(nproc)"

  cp build/coreboot.rom protectli_${BOARD}_${FW_VERSION}.rom
  if [ $? -eq 0 ]; then
    echo "Result binary placed in $PWD/protectli_${BOARD}_${FW_VERSION}.rom"
    sha256sum protectli_${BOARD}_${FW_VERSION}.rom > protectli_${BOARD}_${FW_VERSION}.rom.sha256
  else
    echo "Build failed!"
    exit 1
  fi
}

function build_v1x10 {
  DEFCONFIG="configs/config.protectli_vault_jsl_$1"
  FW_VERSION=$(cat ${DEFCONFIG} | grep CONFIG_LOCALVERSION | cut -d '=' -f 2 | tr -d '"')

  if [ ! -d 3rdparty/dasharo-blobs/protectli ]; then
    git submodule update --init --checkout
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
    "vp66xx" | "VP66XX")
        BOARD="vp66xx"
        build_protectli_vault
        ;;
    "vp46xx" | "VP46XX")
        BOARD="vp46xx"
        build_protectli_vault
        ;;
    "vp2410" | "VP2410")
        BOARD="vp2410"
        build_protectli_vault
        ;;
    "vp2420" | "VP2420")
        BOARD="vp2420"
        build_protectli_vault
        ;;
    "v1210" | "V1210" )
        build_v1x10 "v1210"
        ;;
    "v1410" | "V1410" )
        build_v1x10 "v1410"
        ;;
    "v1610" | "V1610" )
        build_v1x10 "v1610"
        ;;
    *)
        echo "Invalid command: \"$CMD\""
        usage
        ;;
esac
