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
  echo -e "\tvp32xx         - build Dasharo for Protectli VP32xx"
  echo -e "\tvp2430         - build Dasharo for Protectli VP2430"
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

  echo -e "\tv540tu         - build Dasharo for Novacustom V540TU"
  echo -e "\tv560tu         - build Dasharo for Novacustom V560TU"
  echo -e "\tns5x7x         - build Dasharo for Novacustom NS5x/NS7x"
  echo -e "\tnv4x_adl       - build Dasharo for Novacustom NV4x 12 gen"
  echo -e "\tnx5x7x         - build Dasharo for Novacustom Nx5x/Nx7x"
  echo -e "\tnv4x_tgl       - build Dasharo for Novacustom NV4x 11 gen"

  echo "Used environmental variables:"
  echo -e "\tCB_REVISION          - build Dasharo coreboot from a given revision name"
  echo -e "\tNO_DISTCLEAN         - set to not clean the repository before building"
  echo -e "\tNO_CONFIG_OVERRIDE   - set to not override the "./.config" file"
  echo -e "\tNO_SUBMODULE_UPDATE  - set to not update the submodules"
  echo -e "\tEC_REVISION          - use the given EC revvision to build Dasharo coreboot on platforms which require it"
}

SDKVER="2023-11-24_2731fa619b"



function prepare_for_build {
  if [[ -v CB_REVISION ]]; then
    git checkout $CB_REVISION
  fi

  if [[ ! -v NO_DISTCLEAN ]]; then
    docker run --rm -t -u $UID -v $PWD:/home/coreboot/coreboot \
      -v $HOME/.ssh:/home/coreboot/.ssh \
      -w /home/coreboot/coreboot coreboot/coreboot-sdk:$SDKVER \
      /bin/bash -c "make distclean"
  fi

  if [[ ! -v NO_CONFIG_OVERRIDE ]]; then
    cp "${DEFCONFIG}" .config
  fi
}

function build_coreboot {
  TARGET_BINARY=$1
  prepare_for_build

  docker run --rm -t -u $UID -v $PWD:/home/coreboot/coreboot \
    -v $HOME/.ssh:/home/coreboot/.ssh \
    -w /home/coreboot/coreboot coreboot/coreboot-sdk:$SDKVER \
    /bin/bash -c "make olddefconfig && make -j$(nproc)"
  
  cp build/coreboot.rom $TARGET_BINARY
  if [ $? -eq 0 ]; then
    echo "Result binary placed in $PWD/$TARGET_BINARY"
    sha256sum $TARGET_BINARY > $TARGET_BINARY.sha256
  else
    echo "Build failed!"
    exit 1
  fi
}

function build_ec {
  EC_BOARD_VENDOR=$1
  EC_BOARD_MODEL=$2
  if [[ ! -v EC_REVISION ]]; then
    echo "EC_REVISION environmental variable is not set. Please provide an EC revision."
    exit 1;
  fi

  echo "Building EC for $EC_BOARD_VENDOR $EC_BOARD_MODEL from branch $EC_REVISION"
  git clone https://github.com/Dasharo/ec.git &> /dev/null || true
  cd ec
  git checkout $EC_REVISION
  git submodule update --init --recursive --checkout
  EC_BOARD_VENDOR=novacustom EC_BOARD_MODEL=nv4x_adl ./build.sh || true
  cd ..
  cp "ec/${EC_BOARD_VENDOR}_${EC_BOARD_MODEL}_ec.rom" ec.rom
}

function generic_submodules {
  if [[ ! -v NO_SUBMODULE_UPDATE ]]; then
    git submodule update --init --checkout
  fi
}

function protectli_submodules {
  if [[ ! -d 3rdparty/dasharo-blobs/protectli ]]; then
    NO_SUBMODULE_UPDATE="true"
  else
    unset NO_SUBMODULE_UPDATE
  fi
  generic_submodules
}

function pcengines_submodules {
  if [[ ! -v NO_SUBMODULE_UPDATE ]]; then
      # checkout several submodules needed by these boards (some others are checked
      # out by coreboot's Makefile)
      git submodule update --init --force --checkout \
      3rdparty/dasharo-blobs \
      3rdparty/vboot
  fi
}

function qemu_submodules {
  if [[ ! -v NO_SUBMODULE_UPDATE ]]; then
    # checkout several submodules needed by these boards (some others are checked
    # out by coreboot's Makefile)
    git submodule update --init --force --checkout \
        3rdparty/dasharo-blobs
  fi
}

function build_optiplex_9010 {
  DEFCONFIG="configs/config.dell_optiplex_9010_sff_uefi_txt"
  FW_VERSION=v0.1.0-rc1

  generic_submodules
  TARGET_BINARY="${BOARD}_${FW_VERSION}"
  echo "Building Dasharo compatible with Dell OptiPlex 7010/9010 (version $FW_VERSION)"
  build_coreboot $TARGET_BINARY
}

function build_msi {
  DEFCONFIG="configs/config.${BOARD}_$1"
  FW_VERSION=$(cat ${DEFCONFIG} | grep CONFIG_LOCALVERSION | cut -d '=' -f 2 | tr -d '"')

  generic_submodules
  TARGET_BINARY= ${BOARD}_${FW_VERSION}_$1.rom
  echo "Building Dasharo compatible with MSI PRO $2(WIFI) (version $FW_VERSION)"
  build_coreboot $TARGET_BINARY
}

function build_protectli_vault {
  DEFCONFIG="configs/config.protectli_${BOARD}"
  FW_VERSION=$(cat ${DEFCONFIG} | grep CONFIG_LOCALVERSION | cut -d '=' -f 2 | tr -d '"')

  protectli_submodules
  TARGET_BINARY=protectli_${BOARD}_${FW_VERSION}.rom
  echo "Building Dasharo for Protectli $BOARD (version $FW_VERSION)"
  build_coreboot $TARGET_BINARY
}

function build_v1x10 {
  DEFCONFIG="configs/config.protectli_vault_jsl_$1"
  FW_VERSION=$(cat ${DEFCONFIG} | grep CONFIG_LOCALVERSION | cut -d '=' -f 2 | tr -d '"')

  protectli_submodules
  TARGET_BINARY=protectli_$1_${FW_VERSION}.rom
  echo "Building Dasharo for Protectli $1 (version $FW_VERSION)"
  build_coreboot $TARGET_BINARY
}

function build_pcengines {
  VARIANT=$1
  DEFCONFIG="configs/config.pcengines_uefi_${VARIANT}"
  FW_VERSION=$(cat ${DEFCONFIG} | grep CONFIG_LOCALVERSION | cut -d '=' -f 2 | tr -d '"')

  pcengines_submodules
  TARGET_BINARY=pcengines_${VARIANT}_${FW_VERSION}.rom
  echo "Building Dasharo for PC Engines ${VARIANT^^*} (version $FW_VERSION)"
  build_coreboot $TARGET_BINARY
}

function build_qemu {
  DEFCONFIG="configs/config.emulation_qemu_x86_q35_uefi${1:-}"
  FW_VERSION=$(cat ${DEFCONFIG} | grep CONFIG_LOCALVERSION | cut -d '=' -f 2 | tr -d '"')

  qemu_submodules
  TARGET_BINARY=qemu_q35_${FW_VERSION}.rom
  echo "Building Dasharo for QEMU Q35 (version $FW_VERSION)"
  build_coreboot $TARGET_BINARY
}

function build_nv4x_adl {
  DEFCONFIG="configs/config.novacustom_nv4x_adl"
  FW_VERSION=$(cat ${DEFCONFIG} | grep CONFIG_LOCALVERSION | cut -d '=' -f 2 | tr -d '"')

  build_ec novacustom nv4x_adl
  generic_submodules
  TARGET_BINARY=novacustom_nv4x_adl_$FW_VERSION
  echo "Building Dasharo for Novacustom NV4x Adl (version $FW_VERSION)"
  build_coreboot $TARGET_BINARY
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
    "vp32xx" | "VP32XX")
        BOARD="vp32xx"
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
    "vp2430" | "VP2430")
        BOARD="vp2430"
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
    "nv4x_adl" )
        build_nv4x_adl
        ;;
    *)
        echo "Invalid command: \"$CMD\""
        usage
        ;;
esac
