#!/bin/bash

function errorExit {
    errorMessage="$1"
    echo "$errorMessage"
    exit 1
}

function errorCheck {
    errorCode=$?
    errorMessage="$1"
    [ "$errorCode" -ne 0 ] && errorExit "$errorMessage : ($errorCode)"
}

BOARD="clevo_nv41mz"
DEFCONFIG="configs/config.${BOARD}"
FW_VERSION=$(cat ${DEFCONFIG} | grep CONFIG_LOCALVERSION | cut -d '=' -f 2 | tr -d '"')
FW_FILE="dasharo_${BOARD}_${FW_VERSION}.rom"
ARTIFACTS_DIR="${FW_FILE%.*}"

[ -z "$FW_VERSION" ] errorExit "Failed to get FW_VERSION - CONFIG_LOCALVERSION is probably not set"

cp configs/config.clevo_nv41mz .config
make olddefconfig
make -j "$(nproc)"
mkdir -p "${ARTIFACTS_DIR}" 
cp build/coreboot.rom "${ARTIFACTS_DIR}/${FW_FILE}"
sha256sum "${ARTIFACTS_DIR}/${FW_FILE}" > "${ARTIFACTS_DIR}/${FW_FILE}.SHA256"
tar zcvf "${ARTIFACTS_DIR}.tar.gz" "${ARTIFACTS_DIR}"
