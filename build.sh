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
  echo "Available commands: build [-l path_to_logo.bmp] [-s], sign, upload"
  echo
  echo "   -l path_to_logo.bmp: Build with custom boot logo"
  echo "                    -s: Build with new vboot keys for testing vboot"
}

# FIXME
# Novacustom Open Source Firmware Release 0.x Signing Key
GPG_FINGERPRINT="FECB8B01334874A0"

BOARD="novacustom_ns5x"
DEFCONFIG="configs/config.${BOARD}"
FW_VERSION=$(cat ${DEFCONFIG} | grep CONFIG_LOCALVERSION | cut -d '=' -f 2 | tr -d '"')
FW_FILE="dasharo_${BOARD}_${FW_VERSION}.rom"
HASH_FILE="${FW_FILE%.*}.SHA256"
SIG_FILE="${HASH_FILE}.sig"
ARTIFACTS_DIR="artifacts"
LOGO="3rdparty/dasharo-blobs/novacustom/bootsplash.bmp"
SDKVER="0ad5fbd48d"
REPLACE_KEYS=0

[ -z "$FW_VERSION" ] && errorExit "Failed to get FW_VERSION - CONFIG_LOCALVERSION is probably not set"

replace_keys() {
  # Build vboot utilities
  make -C 3rdparty/vboot
  make -C 3rdparty/vboot/ install_for_test
  PATH=$PATH:$PWD/3rdparty/vboot/build/install_for_test/usr/bin

  # Remove existing keys, if present
  if [[ -d "keys" ]]; then
    chmod -R +w keys
    rm -rf keys
  fi

  # Generate new keys
  3rdparty/vboot/scripts/keygeneration/create_new_keys.sh --output keys/
  chmod -R +w keys

  # Remove current keys from config, if present
  sed -i "/\b\(CONFIG_VBOOT_ROOT_KEY\|CONFIG_VBOOT_RECOVERY_KEY\|CONFIG_VBOOT_FIRMWARE_PRIVKEY\|CONFIG_VBOOT_KERNEL_KEY\|CONFIG_VBOOT_KEYBLOCK\)\b/d" .config

  # Add generated keys to .config
  echo "CONFIG_VBOOT_ROOT_KEY=keys/root_key.vbpubk" >> .config
  echo "CONFIG_VBOOT_RECOVERY_KEY=keys/recovery_key.vbpubk" >> .config
  echo "CONFIG_VBOOT_FIRMWARE_PRIVKEY=keys/firmware_data_key.vbprivk" >> .config
  echo "CONFIG_VBOOT_KERNEL_KEY=keys/kernel_subkey.vbpubk" >> .config
  echo "CONFIG_VBOOT_KEYBLOCK=keys/firmware.keyblock" >> .config

  echo "Building with test vboot keys"
}

build() {
  cp "${DEFCONFIG}" .config
  make olddefconfig
  echo "Building with logo $LOGO"
  if [[ $REPLACE_KEYS = 1 ]]; then
    replace_keys
  fi
  make clean
  docker run -u $UID --rm -it -v $PWD:/home/coreboot/coreboot -w /home/coreboot/coreboot \
	  coreboot/coreboot-sdk:$SDKVER make -j "$(nproc)"
  docker run -u $UID --rm -it -v $PWD:/home/coreboot/coreboot -w /home/coreboot/coreboot \
    make -C util/cbfstool
  util/cbfstool/cbfstool build/coreboot.rom add -r BOOTSPLASH -f $LOGO -n logo.bmp -t raw -c lzma
  mkdir -p "${ARTIFACTS_DIR}"
  cp build/coreboot.rom "${ARTIFACTS_DIR}/${FW_FILE}"
  cd "${ARTIFACTS_DIR}"
  sha256sum "${FW_FILE}" > "${HASH_FILE}"
  cd -
}

build-CI() {
  cp "${DEFCONFIG}" .config
  make olddefconfig
  echo "Building with logo $LOGO"
  if [[ $REPLACE_KEYS = 1 ]]; then
    replace_keys
  fi
  make clean
  make -j "$(nproc)"
  make -C util/cbfstool
  util/cbfstool/cbfstool build/coreboot.rom add -r BOOTSPLASH -f $LOGO -n logo.bmp -t raw -c lzma
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
while getopts "l:s" options; do
  case "${options}" in
    "l")
      if [ -f $OPTARG ]; then
        LOGO=$OPTARG
      else
        echo "File $OPTARG does not exist"
      fi
      ;;
    "s")
      REPLACE_KEYS=1
      FW_FILE="${FW_FILE}.vboot_test"
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
