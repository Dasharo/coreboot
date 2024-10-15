#!/bin/bash

set -euo pipefail

usage() {
  echo "${0} CMD"
  echo "Available CMDs:"
  echo -e "\tz690a_ddr4     - build Dasharo image compatible with MSI PRO Z690-A (WIFI) DDR4"
  echo -e "\tz690a_ddr5     - build Dasharo image compatible with MSI PRO Z690-A (WIFI)"
  echo -e "\tz790p_ddr4     - build Dasharo image compatible with MSI PRO Z790-P (WIFI) DDR4"
  echo -e "\tz790p_ddr5     - build Dasharo image compatible with MSI PRO Z790-P (WIFI)"
  echo -e "\tvp66xx         - build Dasharo for Protectli VP66xx"
  echo -e "\tvp46xx         - build Dasharo for Protectli VP46xx"
  echo -e "\tvp2420         - build Dasharo for Protectli VP2420"
  echo -e "\tvp2410         - build Dasharo for Protectli VP2410"
  echo -e "\tV1210          - build Dasharo for Protectli V1210"
  echo -e "\tV1410          - build Dasharo for Protectli V1410"
  echo -e "\tV1610          - build Dasharo for Protectli V1610"
  echo -e "\tapu2           - build Dasharo for PC Engines APU2"
  echo -e "\tapu3           - build Dasharo for PC Engines APU3"
  echo -e "\tapu4           - build Dasharo for PC Engines APU4"
  echo -e "\tapu6           - build Dasharo for PC Engines APU6"
  echo -e "\toptiplex_9010  - build Dasharo compatible with Dell OptiPlex 7010/9010"
  echo -e "\tqemu           - build Dasharo for QEMU Q35"
  echo -e "\tqemu_full      - build Dasharo for QEMU Q35 with all menus available"
  echo -e "\todroid_h4      - build Dasharo compatible with Hardkernel ODROID H4"
}

SDKVER="2023-11-24_2731fa619b"
CONTAINER="ghcr.io/dasharo/dasharo-sdk:latest"


function build_optiplex_9010 {
  DEFCONFIG="configs/config.dell_optiplex_9010_sff_uefi_txt"
  FW_VERSION=v0.1.0-rc1

  docker run --rm -t -u $UID -v $PWD:/home/coreboot/coreboot \
    -v $HOME/.ssh:/home/coreboot/.ssh \
    -w /home/coreboot/coreboot coreboot/coreboot-sdk:$SDKVER \
    /bin/bash -c "make distclean"

  cp "${DEFCONFIG}" .config

  git submodule update --init --checkout

  echo "Building Dasharo compatible with Dell OptiPlex 7010/9010 (version $FW_VERSION)"

  docker run --rm -t -u $UID -v $PWD:/home/coreboot/coreboot \
    -v $HOME/.ssh:/home/coreboot/.ssh \
    -w /home/coreboot/coreboot coreboot/coreboot-sdk:$SDKVER \
    /bin/bash -c "make olddefconfig && make -j$(nproc)"

  cp build/coreboot.rom ${BOARD}_${FW_VERSION}.rom
  if [ $? -eq 0 ]; then
    echo "Result binary placed in $PWD/${BOARD}_${FW_VERSION}.rom"
    sha256sum ${BOARD}_${FW_VERSION}.rom > ${BOARD}_${FW_VERSION}.rom.sha256
  else
    echo "Build failed!"
    exit 1
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
    -w /home/coreboot/coreboot coreboot/coreboot-sdk:$SDKVER \
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

function build_pcengines {
  VARIANT=$1
  DEFCONFIG="configs/config.pcengines_uefi_${VARIANT}"
  FW_VERSION=$(cat ${DEFCONFIG} | grep CONFIG_LOCALVERSION | cut -d '=' -f 2 | tr -d '"')

  # checkout several submodules needed by these boards (some others are checked
  # out by coreboot's Makefile)
  git submodule update --init --force --checkout \
      3rdparty/dasharo-blobs \
      3rdparty/vboot

  docker run --rm -t -u $UID -v $PWD:/home/coreboot/coreboot \
    -v $HOME/.ssh:/home/coreboot/.ssh \
    -w /home/coreboot/coreboot coreboot/coreboot-sdk:$SDKVER \
    /bin/bash -c "make distclean"

  cp $DEFCONFIG .config

  echo "Building Dasharo for PC Engines ${VARIANT^^*} (version $FW_VERSION)"

  docker run --rm -t -u $UID -v $PWD:/home/coreboot/coreboot \
    -v $HOME/.ssh:/home/coreboot/.ssh \
    -w /home/coreboot/coreboot coreboot/coreboot-sdk:$SDKVER \
    /bin/bash -c "make olddefconfig && make -j$(nproc)"

  cp build/coreboot.rom pcengines_${VARIANT}_${FW_VERSION}.rom
  if [ $? -eq 0 ]; then
    echo "Result binary placed in $PWD/pcengines_${VARIANT}_${FW_VERSION}.rom"
    sha256sum pcengines_${VARIANT}_${FW_VERSION}.rom > pcengines_${VARIANT}_${FW_VERSION}.rom.sha256
  else
    echo "Build failed!"
    exit 1
  fi
}

function build_qemu {
  DEFCONFIG="configs/config.emulation_qemu_x86_q35_uefi${1:-}"
  FW_VERSION=$(cat ${DEFCONFIG} | grep CONFIG_LOCALVERSION | cut -d '=' -f 2 | tr -d '"')

  # checkout several submodules needed by these boards (some others are checked
  # out by coreboot's Makefile)
  git submodule update --init --force --checkout \
      3rdparty/dasharo-blobs

  docker run --rm -t -u $UID -v $PWD:/home/coreboot/coreboot \
    -v $HOME/.ssh:/home/coreboot/.ssh \
    -w /home/coreboot/coreboot coreboot/coreboot-sdk:$SDKVER \
    /bin/bash -c "make distclean"

  cp $DEFCONFIG .config

  echo "Building Dasharo for QEMU Q35 (version $FW_VERSION)"

  docker run --rm -t -u $UID -v $PWD:/home/coreboot/coreboot \
    -v $HOME/.ssh:/home/coreboot/.ssh \
    -w /home/coreboot/coreboot coreboot/coreboot-sdk:$SDKVER \
    /bin/bash -c "make olddefconfig && make -j$(nproc)"

  cp build/coreboot.rom qemu_q35_${FW_VERSION}.rom
  if [ $? -eq 0 ]; then
    echo "Result binary placed in $PWD/qemu_q35_${FW_VERSION}.rom"
    sha256sum qemu_q35_${FW_VERSION}.rom > qemu_q35_${FW_VERSION}.rom.sha256
  else
    echo "Build failed!"
    exit 1
  fi
}

function build_odroid_h4 {
  DEFCONFIG="configs/config.hardkernel_odroid_h4"
  FW_VERSION=$(cat ${DEFCONFIG} | grep CONFIG_LOCALVERSION | cut -d '=' -f 2 | tr -d '"')

  # checkout several submodules needed by these boards (some others are checked
  # out by coreboot's Makefile)
  git submodule update --init --force --checkout \
      3rdparty/dasharo-blobs

  docker run --rm -t -u $UID -v $PWD:/home/coreboot/coreboot \
    -v $HOME/.ssh:/home/coreboot/.ssh \
    -w /home/coreboot/coreboot ${CONTAINER} \
    /bin/bash -c "make distclean"

  cp $DEFCONFIG .config

  echo "Building Dasharo compatbile with Hardkernel ODROID H4 (version $FW_VERSION)"

  docker run --rm -t -u $UID -v $PWD:/home/coreboot/coreboot \
    -v $HOME/.ssh:/home/coreboot/.ssh \
    -w /home/coreboot/coreboot ${CONTAINER} \
    /bin/bash -c "make olddefconfig && make -j$(nproc)"

  cp build/coreboot.rom hardkernel_odroid_h4_${FW_VERSION}.rom
  if [ $? -eq 0 ]; then
    echo "Result binary placed in $PWD/hardkernel_odroid_h4_${FW_VERSION}.rom"
    sha256sum hardkernel_odroid_h4_${FW_VERSION}.rom > hardkernel_odroid_h4_${FW_VERSION}.rom.sha256
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
    "v1211" | "V1211" )
        build_v1x10 "v1211"
        ;;
    "v1410" | "V1410" )
        build_v1x10 "v1410"
        ;;
    "v1610" | "V1610" )
        build_v1x10 "v1610"
        ;;
    "apu2" | "APU2" )
        build_pcengines "apu2"
        ;;
    "apu3" | "APU3" )
        build_pcengines "apu3"
        ;;
    "apu4" | "APU4" )
        build_pcengines "apu4"
        ;;
    "apu6" | "APU6" )
        build_pcengines "apu6"
        ;;
    "optiplex_9010" | "optiplex_7010" )
        BOARD=optiplex_9010
        build_optiplex_9010
        ;;
    "qemu" | "QEMU" | "q35" | "Q35" )
        build_qemu
        ;;
    "qemu_full" | "QEMU_full" | "q35_full" | "Q35_full" )
        build_qemu "_all_menus"
        ;;
    "odroid_h4" | "odroid_H4" | "ODROID_H4" )
        build_odroid_h4
        ;;
    *)
        echo "Invalid command: \"$CMD\""
        usage
        ;;
esac
