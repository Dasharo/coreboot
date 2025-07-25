#!/bin/bash
# A script to generate cabinets for fwupd / LVFS

set -e

edk_workspace=${EDK_WORKSPACE:-payloads/external/edk2/workspace/Dasharo}
edk_tools=${edk_workspace}/BaseTools/BinWrappers/PosixLike

function die() {
    echo error: "$@" 1>&2
    exit 1
}

if [ $# -ne 1 ]; then
    die "Incorrect number of input parameters specified: $# (expected: 1)"
fi

if [ -z $1 ]; then
    die "No input capsule specified"
fi

if [ ! -f $1 ]; then
    die "File $1 not found"
fi

if [ ! -f .config ]; then
    die "No '.config' file in current directory"
fi

capsule=$1
date=$(stat -c %w $capsule | cut -d ' ' -f 1)
vendor=$(cat .config | grep -e "CONFIG_VENDOR_.*=y" | cut -d '=' -f 1 | cut -d '_' -f 3- | awk '{ print tolower($0) }')
version=$(echo $CONFIG_LOCALVERSION | tr -d 'v' | cut -d '-' -f 1)

archive_dir=$(mktemp --tmpdir -d XXXXXXXX)

cat > "${archive_dir}/firmware.metainfo.xml" << EOF
<?xml version='1.0' encoding='utf-8'?>
<component type="firmware">
  <id>com.${vendor}.${CONFIG_MAINBOARD_SMBIOS_PRODUCT_NAME}.${CONFIG_MAINBOARD_VERSION}.system.firmware</id>
  <name>${CONFIG_MAINBOARD_SMBIOS_PRODUCT_NAME}</name>
  <summary>${CONFIG_MAINBOARD_SMBIOS_PRODUCT_NAME} ${CONFIG_MAINBOARD_VERSION} system firmware</summary>
  <description>
    <p>Dasharo ${CONFIG_MAINBOARD_SMBIOS_PRODUCT_NAME} ${CONFIG_MAINBOARD_VERSION} system firmware</p>
  </description>
  <provides>
    <firmware type="flashed">${CONFIG_DRIVERS_EFI_MAIN_FW_GUID}</firmware>
  </provides>
  <url type="homepage">https://docs.dasharo.com/</url>
  <metadata_license>CC0-1.0</metadata_license>
  <project_license>LicenseRef-proprietary</project_license>
  <categories>
    <category>X-System</category>
  </categories>
  <custom>
    <value key="LVFS::VersionFormat">quad</value>
    <value key="LVFS::VersionFormat">dell-bios-msb</value>
    <value key="LVFS::UpdateProtocol">org.uefi.capsule</value>
  </custom>
  <releases>
    <release version="${version}" date="${date}" tag="${CONFIG_LOCALVERSION}" urgency="high">
      <checksum filename="firmware.bin" target="content"/>
    </release>
  </releases>
</component>

EOF

decode_dir=$(mktemp --tmpdir -d XXXXXXXX)
if ! "${edk_tools}/GenerateCapsule" --decode $capsule --output $decode_dir/capsule; then
  die "GenerateCapsule --decode failed!"
fi
cbfstool $decode_dir/capsule.Payload.1.bin extract -r COREBOOT -n config -f $decode_dir/config

# import coreboot's config file replacing $(...) with ${...}
while read -r line; do
    if ! eval "$line"; then
        die "failed to source 'config'"
    fi
done <<< "$(sed 's/\$(\([^)]\+\))/${\1}/g' $decode_dir/config)"

rm -r $decode_dir

if [ "$CONFIG_DRIVERS_EFI_UPDATE_CAPSULES" != y ]; then
    die "Current board configuration lacks support of update capsules"
fi

cp $capsule $archive_dir/firmware.bin

pushd $archive_dir &> /dev/null
fwupdtool build-cabinet $capsule.cab firmware.bin firmware.metainfo.xml
popd &> /dev/null

cp $archive_dir/$capsule.cab ./

echo "File $capsule.cab created"

rm -r $archive_dir
