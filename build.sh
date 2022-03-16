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

# FIXME
# Novacustom Open Source Firmware Release 0.x Signing Key
GPG_FINGERPRINT="FECB8B01334874A0"

BOARD="tuxedo_ibs15"
DEFCONFIG="configs/config.${BOARD}"
FW_VERSION=$(cat ${DEFCONFIG} | grep CONFIG_LOCALVERSION | cut -d '=' -f 2 | tr -d '"')
FW_FILE="dasharo_${BOARD}_${FW_VERSION}.rom"
HASH_FILE="${FW_FILE%.*}.SHA256"
SIG_FILE="${HASH_FILE}.sig"
ARTIFACTS_DIR="artifacts"
LOGO=""
SDKVER="0ad5fbd48d"
BUILD_TIMELESS=1

CPUS=$(nproc)
NUM_CPUS=$CPUS

[ -z "$FW_VERSION" ] && errorExit "Failed to get FW_VERSION - CONFIG_LOCALVERSION is probably not set"

replace_logo() {
  path=custom_bootsplash.bmp
  cp $1 $path

  if [[ $(grep "CONFIG_TIANOCORE_BOOTSPLASH_IMAGE" .config; echo $?) == 1 ]]; then
    echo "CONFIG_TIANOCORE_BOOTSPLASH_IMAGE=y" >> .config
  else
    sed -i "/CONFIG_TIANOCORE_BOOTSPLASH_IMAGE/c\CONFIG_TIANOCORE_BOOTSPLASH_IMAGE=y" .config
  fi

  if [[ $(grep "CONFIG_TIANOCORE_BOOTSPLASH_FILE" .config; echo $?) == 1 ]]; then
    echo "CONFIG_TIANOCORE_BOOTSPLASH_FILE=\"$path\"" >> .config
  else
    sed -i "/CONFIG_TIANOCORE_BOOTSPLASH_FILE/c\CONFIG_TIANOCORE_BOOTSPLASH_FILE=\"$path\"" .config
  fi
}

build() {
  cp "${DEFCONFIG}" .config
  make olddefconfig
  if [[ -n $LOGO ]]; then
    echo "Building with custom logo $LOGO"
    replace_logo $LOGO
  else
    echo "Building with default logo"
  fi
  make clean
  docker run -u $UID --rm -it -e BUILD_TIMELESS=$BUILD_TIMELESS \
    -e CPUS=$CPUS -e NUM_CPUS=$NUM_CPUS \
    -v $PWD:/home/coreboot/coreboot \
    -w /home/coreboot/coreboot \
    coreboot/coreboot-sdk:$SDKVER make -j "$(nproc)"
  mkdir -p "${ARTIFACTS_DIR}"
  cp build/coreboot.rom "${ARTIFACTS_DIR}/${FW_FILE}"
  cd "${ARTIFACTS_DIR}"
  sha256sum "${FW_FILE}" > "${HASH_FILE}"
  cd -
}

build-CI() {
  cp "${DEFCONFIG}" .config
  make olddefconfig
  if [[ -n $LOGO ]]; then
    echo "Building with custom logo $LOGO"
    replace_logo $LOGO
  else
    echo "Building with default logo"
  fi
  make clean
  make -j "$(nproc)" BUILD_TIMELESS=$BUILD_TIMELESS
  mkdir -p "${ARTIFACTS_DIR}"
  cp build/coreboot.rom "${ARTIFACTS_DIR}/${FW_FILE}"
  sha256sum "${ARTIFACTS_DIR}/${FW_FILE}" > "${ARTIFACTS_DIR}/${HASH_FILE}"
}

sign() {
  gpg --default-key "${GPG_FINGERPRINT}" --armor --output "${ARTIFACTS_DIR}/${SIG_FILE}" --detach-sig "${ARTIFACTS_DIR}/${HASH_FILE}"
}

upload() {
  if (git describe --exact-match --tags)
  then
    REMOTE_DIR="/projects/Tuxedo/releases/ibs15/${FW_VERSION}"
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
while getopts "l" options; do
  case "${options}" in
    "l")
      if [ -f $3 ]; then
        LOGO=$3
      else
        echo "File $3 does not exist"
      fi
      ;;
  esac
done

case "$CMD" in
    "build")
        build
        ;;
    "build-CI")
        build-CI
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
