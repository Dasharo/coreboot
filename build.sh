#!/bin/bash

set -euo pipefail

usage() {
  echo "${0} CMD"
  echo "Available CMDs:"
  echo -e "\tz690a_ddr4             - build Dasharo image compatible with MSI PRO Z690-A (WIFI) DDR4"
  echo -e "\tz690a_ddr5             - build Dasharo image compatible with MSI PRO Z690-A (WIFI)"
  echo -e "\tz790p_ddr4             - build Dasharo image compatible with MSI PRO Z790-P (WIFI) DDR4"
  echo -e "\tz790p_ddr5             - build Dasharo image compatible with MSI PRO Z790-P (WIFI)"
  echo -e "\tvp66xx                 - build Dasharo for Protectli VP66xx"
  echo -e "\tvp46xx                 - build Dasharo for Protectli VP46xx"
  echo -e "\tvp46xx_noemmc- build Dasharo for Protectli VP46xx variants without eMMC (VP46xxe, VP4651)"
  echo -e "\tvp32xx                 - build Dasharo for Protectli VP32xx"
  echo -e "\tvp2440                 - build Dasharo for Protectli VP2440"
  echo -e "\tvp2440_noemmc          - build Dasharo for Protectli VP2440 without eMMC (VP2440e)"
  echo -e "\tvp2430                 - build Dasharo for Protectli VP2430"
  echo -e "\tvp2420                 - build Dasharo for Protectli VP2420"
  echo -e "\tvp2410                 - build Dasharo for Protectli VP2410"
  echo -e "\tV1210                  - build Dasharo for Protectli V1210"
  echo -e "\tV1211                  - build Dasharo for Protectli V1211"
  echo -e "\tV1410                  - build Dasharo for Protectli V1410"
  echo -e "\tV1610                  - build Dasharo for Protectli V1610"
  echo -e "\tns5x_adl               - build Dasharo for Novacustom NS5x_ADL"
  echo -e "\tns5x_tgl               - build Dasharo for Novacustom NS5x_TGL"
  echo -e "\tnv4x_adl               - build Dasharo for Novacustom NV4x_ADL"
  echo -e "\tnv4x_tgl               - build Dasharo for Novacustom NV4x_TGL"
  echo -e "\tv540tnx                - build Dasharo for Novacustom V540TNx"
  echo -e "\tv540tu                 - build Dasharo for Novacustom V540TU"
  echo -e "\tv560tnx                - build Dasharo for Novacustom V560TNx"
  echo -e "\tv560tu                 - build Dasharo for Novacustom V560TU"
  echo -e "\tnuc_box                - build Dasharo for Novacustom NUC BOX"
  echo -e "\tapu2                   - build Dasharo for PC Engines APU2"
  echo -e "\tapu3                   - build Dasharo for PC Engines APU3"
  echo -e "\tapu4                   - build Dasharo for PC Engines APU4"
  echo -e "\tapu6                   - build Dasharo for PC Engines APU6"
  echo -e "\toptiplex_9010_uefi     - build Dasharo compatible with Dell OptiPlex 7010/9010 (UEFI)"
  echo -e "\toptiplex_9010_seabios  - build Dasharo compatible with Dell OptiPlex 7010/9010 (SeaBIOS)"
  echo -e "\tqemu                   - build Dasharo for QEMU Q35"
  echo -e "\tqemu_full              - build Dasharo for QEMU Q35 with all menus available"
  echo -e "\todroid_h4              - build Dasharo compatible with Hardkernel ODROID H4"
  echo -e "\todroid_h4_netcard      - build Dasharo compatible with Hardkernel ODROID H4 for netcard support"
  echo -e "\tasrock_spc741d8        - build Dasharo compatible with ASRock Rack SPC741D8-2L2T/BCM"
}

DASHARO_SDK=${DASHARO_SDK:-"ghcr.io/dasharo/dasharo-sdk:v1.9.0"}
BUILD_TIMELESS=${BUILD_TIMELESS:-0}
AIRGAP=${AIRGAP:-0}

function sdk_run {
  docker run --rm -t -u $UID -v $PWD:/home/coreboot/coreboot \
    -v $HOME/.ssh:/home/coreboot/.ssh \
    -e BUILD_TIMELESS=${BUILD_TIMELESS} \
    -w /home/coreboot/coreboot ${DASHARO_SDK} \
    "$@"
}

function build_prep {
  if [ "${AIRGAP}" -eq 1 ]; then
    sdk_run /bin/bash -c "make clean"
  else
    sdk_run /bin/bash -c "make distclean"
  fi

  cp "${DEFCONFIG}" .config

  git submodule update --init --checkout $@
}

function build_start {
  if [ "${AIRGAP}" -eq 1 ]; then

    # In this situation we assume that provided repository is ready to be used
    # and nothing should be downloaded during build process.

    if [ -d "${EDK2_REPO_PATH}" ]; then
      # Without following sequence workspce would be created by docker with root
      # privilidges and build will fail.
      # Target directory
      TARGET_DIR="payloads/external/edk2/workspace/Dasharo"
      mkdir -p "$TARGET_DIR"
      chown -R $(id -u):$(id -g) "$TARGET_DIR"
      chmod -R 755 "$TARGET_DIR"
      sdk_run --network none \
        ${EDK2_REPO_PATH:+-v $EDK2_REPO_PATH:/home/coreboot/coreboot/${TARGET_DIR}} \
        /bin/bash -c "make olddefconfig && make -j$(nproc)"
    else
      echo "EDK2_REPO_PATH is not defined in AIRGAP!"
      exit 1
    fi
  else
    sdk_run /bin/bash -c "make olddefconfig && make -j$(nproc)"
  fi
}

function build_optiplex_9010 {
  DEFCONFIG=$1
  # Determine FW flavor first (uefi or seabios)
  if [[ ${DEFCONFIG} == *"uefi"* ]]; then
    FW_FLAVOR="uefi"
  else
    FW_FLAVOR="seabios"
  fi

  # Get FW version
  FW_VERSION=$(cat ${DEFCONFIG} | grep CONFIG_LOCALVERSION | cut -d '=' -f 2 | tr -d '"')

  # Combine FW flavor with version
  FW_VERSION="${FW_FLAVOR}_${FW_VERSION}"

  build_prep

  echo "Building Dasharo compatible with Dell OptiPlex 7010/9010 (version $FW_VERSION)"

  build_start

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

  build_prep

  echo "Building Dasharo compatible with MSI PRO $2(WIFI) (version $FW_VERSION)"

  build_start

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
  EMMC_VARIANT="${1:-""}"
  DEFCONFIG="configs/config.protectli_${BOARD}${EMMC_VARIANT}"
  FW_VERSION=$(cat ${DEFCONFIG} | grep CONFIG_LOCALVERSION | cut -d '=' -f 2 | tr -d '"')

  build_prep

  echo "Building Dasharo for Protectli ${BOARD}${EMMC_VARIANT} (version $FW_VERSION)"

  build_start

  cp build/coreboot.rom protectli_${BOARD}${EMMC_VARIANT}_${FW_VERSION}.rom

  if [ $? -eq 0 ]; then
    echo "Result binary placed in $PWD/protectli_${BOARD}${EMMC_VARIANT}_${FW_VERSION}.rom"
    sha256sum protectli_${BOARD}${EMMC_VARIANT}_${FW_VERSION}.rom > protectli_${BOARD}${EMMC_VARIANT}_${FW_VERSION}.rom.sha256
  else
    echo "Build failed!"
    exit 1
  fi
}

function build_v1x10 {
  DEFCONFIG="configs/config.protectli_vault_jsl_$1"
  FW_VERSION=$(cat ${DEFCONFIG} | grep CONFIG_LOCALVERSION | cut -d '=' -f 2 | tr -d '"')

  build_prep

  echo "Building Dasharo for Protectli $1 (version $FW_VERSION)"

  build_start

  cp build/coreboot.rom protectli_$1_${FW_VERSION}.rom

  if [ $? -eq 0 ]; then
    echo "Result binary placed in $PWD/protectli_$1_${FW_VERSION}.rom"
    sha256sum protectli_$1_${FW_VERSION}.rom > protectli_$1_${FW_VERSION}.rom.sha256
  else
    echo "Build failed!"
    exit 1
  fi
}

function build_novacustom {
  DEFCONFIG="configs/config.novacustom_$1"
  FW_VERSION=$(cat ${DEFCONFIG} | grep CONFIG_LOCALVERSION | cut -d '=' -f 2 | tr -d '"')

  build_prep

  echo "Building Dasharo for Novacustom $1 (version $FW_VERSION)"

  build_start

  cp build/coreboot.rom novacustom_$1_${FW_VERSION}.rom

  if [ $? -eq 0 ]; then
    echo "Result binary placed in $PWD/novacustom_$1_${FW_VERSION}.rom"
    sha256sum novacustom_$1_${FW_VERSION}.rom > novacustom_$1_${FW_VERSION}.rom.sha256
  else
    echo "Build failed!"
    exit 1
  fi
}


function build_novacustom_v5x0tu {
  DEFCONFIG="configs/config.novacustom_$1"
  FW_VERSION=$(cat ${DEFCONFIG} | grep CONFIG_LOCALVERSION | cut -d '=' -f 2 | tr -d '"')

  build_prep

  # Obtain LAN ROM blob from release binary
  wget -O UEFIExtract_NE_A68_x64_linux.zip https://github.com/LongSoft/UEFITool/releases/download/A68/UEFIExtract_NE_A68_x64_linux.zip
  unzip -o UEFIExtract_NE_A68_x64_linux.zip
  wget -O novacustom_v54x_mtl_v0.9.0.rom https://dl.3mdeb.com/open-source-firmware/Dasharo/novacustom_v54x_mtl/v0.9.0/novacustom_v54x_mtl_v0.9.0.rom

  # Extract and transfer LAN ROM blob
  docker run --rm -t -u $UID -v $PWD:/home/coreboot/coreboot \
    -v $HOME/.ssh:/home/coreboot/.ssh \
    -w /home/coreboot/coreboot ${DASHARO_SDK}  \
    /bin/bash -c "make -C util/cbfstool && \
    util/cbfstool/cbfstool novacustom_v54x_mtl_v0.9.0.rom extract -r COREBOOT -f payload -n fallback/payload -m x86"

  ./uefiextract payload DEB917C0-C56A-4860-A05B-BF2F22EBB717
  mkdir -p 3rdparty/blobs/mainboard/novacustom/mtl-h
  cp payload.dump/2\ 8C8CE578-8A3D-4F1C-9935-896185C32DD3/82\ DEB917C0-C56A-4860-A05B-BF2F22EBB717/1\ PE32\ image\ section/body.bin 3rdparty/blobs/mainboard/novacustom/mtl-h/LanRom.efi
  rm -rf payload.dump

  echo "Building Dasharo for Novacustom $1 (version $FW_VERSION)"

  build_start

  cp build/coreboot.rom novacustom_$1_${FW_VERSION}.rom

  if [ $? -eq 0 ]; then
    echo "Result binary placed in $PWD/novacustom_$1_${FW_VERSION}.rom"
    sha256sum novacustom_$1_${FW_VERSION}.rom > novacustom_$1_${FW_VERSION}.rom.sha256
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
  build_prep 3rdparty/dasharo-blobs 3rdparty/vboot

  echo "Building Dasharo for PC Engines ${VARIANT^^*} (version $FW_VERSION)"

  build_start

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
  build_prep 3rdparty/dasharo-blobs

  echo "Building Dasharo for QEMU Q35 (version $FW_VERSION)"

  build_start

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
  VARIANT=$1
  DEFCONFIG="configs/config.hardkernel_${VARIANT}"
  FW_VERSION=$(cat ${DEFCONFIG} | grep CONFIG_LOCALVERSION | cut -d '=' -f 2 | tr -d '"')

  # checkout several submodules needed by these boards (some others are checked
  # out by coreboot's Makefile)
  build_prep 3rdparty/dasharo-blobs

  echo "Building Dasharo compatbile with Hardkernel ODROID H4 (version $FW_VERSION)"

  build_start

  cp build/coreboot.rom hardkernel_${VARIANT}_${FW_VERSION}.rom
  if [ $? -eq 0 ]; then
    echo "Result binary placed in $PWD/hardkernel_${VARIANT}_${FW_VERSION}.rom"
    sha256sum hardkernel_${VARIANT}_${FW_VERSION}.rom > hardkernel_${VARIANT}_${FW_VERSION}.rom.sha256
  else
    echo "Build failed!"
    exit 1
  fi
}

function build_asrock_rack {
  DEFCONFIG="configs/config.asrock_spc741d8"
  FW_VERSION=$(cat ${DEFCONFIG} | grep CONFIG_LOCALVERSION | cut -d '=' -f 2 | tr -d '"')

  build_prep

  echo "Building Dasharo for ASRock Rack SPC741D8-2L2T/BCM (version $FW_VERSION)"

  build_start

  cp build/coreboot.rom asrock_spc741d8_${FW_VERSION}.rom

  if [ $? -eq 0 ]; then
    echo "Result binary placed in $PWD/asrock_spc741d8_${FW_VERSION}.rom"
    sha256sum asrock_spc741d8_${FW_VERSION}.rom > asrock_spc741d8_${FW_VERSION}.rom.sha256
  else
    echo "Build failed!"
    exit 1
  fi
}

if [ $# -lt 1 ]; then
  usage
  exit
fi

CMD="$1"

case "$CMD" in
    "ddr4" | "z690a_ddr4" | "ms7d25_ddr4")
        BOARD="msi_ms7d25"
        build_msi ddr4 "Z690-A DDR4 "
        ;;
    "ddr5" | "z690a_ddr5" | "ms7d25_ddr5")
        BOARD="msi_ms7d25"
        build_msi ddr5 "Z690-A DDR5 "
        ;;
    "z790p_ddr4" | "ms7e06_ddr4")
        BOARD="msi_ms7e06"
        build_msi ddr4 "Z790-P DDR4 "
        ;;
    "z790p_ddr5" | "ms7e06_ddr5")
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
    "vp46xx_noemmc" | "VP46XX_noemmc" | "vp46xxe" | "VP46XXe")
        BOARD="vp46xx"
        build_protectli_vault _no_emmc
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
    "vp2440" | "VP2440")
        BOARD="vp2440"
        build_protectli_vault
        ;;
    "vp2440_noemmc" | "VP2440_noemmc" | "vp2440e" | "VP2440e")
        BOARD="vp2440"
        build_protectli_vault _no_emmc
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
    "v540tnx" | "V540TNx" | "V540TNX" )
        BOARD="v540tnx"
        build_novacustom "v540tnx"
        ;;
    "v560tnx" | "V560TNx" | "V560TNX" )
        BOARD="v560tnx"
        build_novacustom "v560tnx"
        ;;
    "nv4x_adl" | "NV4x_ADL" | "NV4X_ADL" )
        BOARD="nv4x_adl"
        build_novacustom "nv4x_adl"
        ;;
    "nv4x_tgl" | "NV4x_tgl" | "NV4X_TGL" )
        BOARD="nv4x_tgl"
        build_novacustom "nv4x_tgl"
        ;;
    "ns5x_adl" | "NS5x_ADL" | "NS5X_ADL" )
        BOARD="ns5x_adl"
        build_novacustom "ns5x_adl"
        ;;
    "ns5x_tgl" | "NS5x_TGL" | "NS5X_TGL" )
        BOARD="ns5x_tgl"
        build_novacustom "ns5x_tgl"
        ;;
    "v560tu" | "V560TU" )
        BOARD="v560tu"
        build_novacustom_v5x0tu "v560tu"
        ;;
    "nuc_box" | "nucbox" )
        BOARD="nuc_box"
        build_novacustom "nuc_box"
        ;;
    "v540tu" | "V540TU" )
        BOARD="v540tu"
        build_novacustom_v5x0tu "v540tu"
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
    "optiplex_9010_uefi" | "optiplex_9010_sff_uefi")
        BOARD="optiplex_9010"
        build_optiplex_9010 "configs/config.dell_optiplex_9010_sff_uefi_txt"
        ;;
    "optiplex_9010_seabios" | "optiplex_9010_sff")
        BOARD="optiplex_9010"
        build_optiplex_9010 "configs/config.dell_optiplex_9010_sff_txt"
        ;;
    "qemu" | "QEMU" | "q35" | "Q35" | "x86_q35_uefi" )
        build_qemu
        ;;
    "qemu_full" | "QEMU_full" | "q35_full" | "Q35_full" | "x86_q35_uefi_all_menus" )
        build_qemu "_all_menus"
        ;;
    "odroid_h4" | "odroid_H4" | "ODROID_H4" )
        build_odroid_h4 "odroid_h4"
        ;;
    "odroid_h4_netcard" | "odroid_H4_netcard" | "ODROID_H4_NETCARD" )
        build_odroid_h4 "odroid_h4_netcard"
        ;;
    "asrock_spc741d8")
        build_asrock_rack
        ;;
    *)
        echo "Invalid command: \"$CMD\""
        usage
        ;;
esac
