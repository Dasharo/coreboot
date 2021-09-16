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
  echo "Available commands: build, sign, upload"
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

upload() {
  REMOTE_DIR="/projects/novacustom/releases/${FW_VERSION}"
  FILES="${ARTIFACTS_DIR}/*"
  curl -s -u $UPLOADER_USERNAME:$UPLOADER_PASSWORD -X MKCOL "${UPLOADER_URL}${REMOTE_DIR}"
  for f in $FILES
  do
    f=$(basename $f)
    curl -s -u $UPLOADER_USERNAME:$UPLOADER_PASSWORD -T $ARTIFACTS_DIR/$f "${UPLOADER_URL}${REMOTE_DIR}/$f"
    f=${f/+/%2B}
    GENERATED_URL=$(curl -s -u $UPLOADER_USERNAME:$UPLOADER_PASSWORD \
               -X POST 'https://cloud.3mdeb.com/ocs/v2.php/apps/files_sharing/api/v1/shares' \
               -H "OCS-APIRequest: true" \
               -d "path=${REMOTE_DIR:1}/$f&shareType=3" | grep url | sed -e "s#</url>##" | sed -e "s#<url>##" | tr -d '[:space:]' )
    echo $GENERATED_URL
  done
}

CMD="$1"

case "$CMD" in
    "build")
        build
        ;;
    "sign")
        sign
        ;;
    "upload")
        upload
        ;;
    *)
        echo "Invalid command: \"$CMD\""
        usage
        ;;
esac
