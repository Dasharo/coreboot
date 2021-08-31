#!/bin/bash

set -euo pipefail

errorExit() {
    errorMessage="$1"
    echo "$errorMessage"
    exit 1
}

errorCheck() {
    errorCode=$?
    errorMessage="$1"
    [ "$errorCode" -ne 0 ] && errorExit "$errorMessage : ($errorCode)"
}

usage() {
  echo "${0} command"
  echo "Available commands: build, sign"
  exit 0
}

# Novacustom Open Source Firmware Release 0.x Signing Key
GPG_FINGERPRINT="FECB8B01334874A0"

BOARD="clevo_nv41mz"
DEFCONFIG="configs/config.${BOARD}"
FW_VERSION=$(cat ${DEFCONFIG} | grep CONFIG_LOCALVERSION | cut -d '=' -f 2 | tr -d '"')
FW_FILE="dasharo_${BOARD}_${FW_VERSION}.rom"
HASH_FILE="${FW_FILE%.*}.SHA256"
SIG_FILE="${HASH_FILE}.sig"
ARTIFACTS_DIR="artifacts"

[ -z "$FW_VERSION" ] && errorExit "Failed to get FW_VERSION - CONFIG_LOCALVERSION is probably not set"

build() {
  cp "${DEFCONFIG}" .config
  make olddefconfig
  make -j "$(nproc)"
  mkdir -p "${ARTIFACTS_DIR}"
  cp build/coreboot.rom "${ARTIFACTS_DIR}/${FW_FILE}"
  sha256sum "${ARTIFACTS_DIR}/${FW_FILE}" > "${ARTIFACTS_DIR}/${HASH_FILE}"
}

sign() {
  gpg --default-key "${GPG_FINGERPRINT}" --armor --output "${ARTIFACTS_DIR}/${SIG_FILE}" --detach-sig "${ARTIFACTS_DIR}/${HASH_FILE}"
}

CMD="$1"

case "$CMD" in
    "build")
        build
	;;
    "sign")
        sign
        ;;
    *)
        echo "Invalid command: \"$CMD\""
        usage
        ;;
esac
