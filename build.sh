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
  echo "Available commands: build [-l path_to_logo.bmp], sign, upload"
  exit 0
}

# Novacustom Open Source Firmware Release 0.x Signing Key
GPG_FINGERPRINT="FECB8B01334874A0"

BOARD="clevo_nv41mz"
DEFCONFIG="configs/config.${BOARD}"
FW_VERSION=$(cat ${DEFCONFIG} | grep CONFIG_LOCALVERSION | cut -d '=' -f 2 | tr -d '"')
FW_FILE="dasharo_${BOARD}_${FW_VERSION}.rom"
HASH_FILE="${FW_FILE}.SHA256"
SIG_FILE="${HASH_FILE}.sig"
ARTIFACTS_DIR="artifacts"
LOGO="3rdparty/dasharo-blobs/novacustom/bootsplash.bmp"

[ -z "$FW_VERSION" ] && errorExit "Failed to get FW_VERSION - CONFIG_LOCALVERSION is probably not set"

build() {
  cp "${DEFCONFIG}" .config
  make olddefconfig
  echo "Building with logo $LOGO"
  make clean
  make -j "$(nproc)"
  make -C util/cbfstool
  util/cbfstool/cbfstool build/coreboot.rom add -r BOOTSPLASH -f $LOGO -n logo.bmp -t raw -c lzma
  mkdir -p "${ARTIFACTS_DIR}"
  cp build/coreboot.rom "${ARTIFACTS_DIR}/${FW_FILE}"
  cd "${ARTIFACTS_DIR}"
  sha256sum "${FW_FILE}" > "${HASH_FILE}"
  cd -
}

sign() {
  cd "${ARTIFACTS_DIR}"
  gpg --default-key "${GPG_FINGERPRINT}" --armor --output "${SIG_FILE}" --detach-sig "${HASH_FILE}"
  cd -
}

upload() {
  if (git describe --exact-match --tags)
  then
    REMOTE_DIR="/projects/novacustom/releases/${FW_VERSION}"
    FILES="${ARTIFACTS_DIR}/*"
    curl --fail -s -u $UPLOADER_USERNAME:$UPLOADER_PASSWORD -X MKCOL "${UPLOADER_URL}${REMOTE_DIR}"
    rm share_urls.txt && touch share_urls.txt
    for f in $FILES
    do
      f=$(basename $f)
      curl --fail -s -u $UPLOADER_USERNAME:$UPLOADER_PASSWORD -T $ARTIFACTS_DIR/$f "${UPLOADER_URL}${REMOTE_DIR}/$f"
      f=${f/+/%2B}
      GENERATED_URL=$(curl -s -u $UPLOADER_USERNAME:$UPLOADER_PASSWORD \
                 -X POST 'https://cloud.3mdeb.com/ocs/v2.php/apps/files_sharing/api/v1/shares' \
                 -H "OCS-APIRequest: true" \
                 -d "path=${REMOTE_DIR:1}/$f&shareType=3" | grep url | sed -e "s#</url>##" | sed -e "s#<url>##" | tr -d '[:space:]' )
      echo $f $GENERATED_URL >> share_urls.txt
    done
    cat share_urls.txt
  else
    echo "Not on a tag, not uploading."
  fi
}

CMD="$1"

OPTIND=2
while getopts "ln:" options; do
  case "${options}" in
    "l")
      if [ -f $3 ]; then
        LOGO=$3
      else
        echo "File $3 does not exist"
      fi
      ;;
    "n")
      if [ ! -z "${OPTARG}" ]; then
        FW_FILE="${OPTARG}"
      else
        echo "Invalid filename. Using default $FW_FILE"
      fi
  esac
done

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
