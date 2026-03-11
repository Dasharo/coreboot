#!/bin/bash
# Generate a firmware update cabinet compatible with both LVFS/fwupd and Windows Update.
#
# The cabinet always contains:
#   firmware.bin           - the capsule payload
#   firmware.metainfo.xml  - LVFS/fwupd metadata
#   firmware.inf           - Windows Update driver package
#   firmware.cat           - Windows Security Catalog (if --cert/--key or --gen-cert given)

set -e

function die() {
    echo error: "$@" 1>&2
    exit 1
}

function usage() {
    echo "Usage: $0 [--cert cert.pem --key key.pem | --gen-cert] <capsule>" 1>&2
    echo "" 1>&2
    echo "  --cert cert.pem    Signing certificate for .cat generation (PEM, may be a chain)" 1>&2
    echo "  --key  key.pem     Signing private key for .cat generation (PEM)" 1>&2
    echo "  --gen-cert         Generate a self-signed code-signing certificate and use it" 1>&2
    echo "                     to sign the .cat; saved as <product>-signing.{crt,key}." 1>&2
    echo "                     For test-signing only — enable on Windows with:" 1>&2
    echo "                       bcdedit /set testsigning on" 1>&2
    echo "" 1>&2
    echo "  For Windows Update submission the .cat must be signed by Microsoft;" 1>&2
    echo "  use Microsoft Hardware Dev Center or provide an EV code-signing cert." 1>&2
    echo "" 1>&2
    echo "  Requires: gcab, python3 cryptography (pip install cryptography)" 1>&2
    exit 1
}

gen_cert=false
capsule=""
cert=""
key=""

while [[ $# -gt 0 ]]; do
    case "$1" in
        --gen-cert)
            gen_cert=true
            shift
            ;;
        --cert)
            [ $# -ge 2 ] || die "--cert requires an argument"
            cert="$2"
            shift 2
            ;;
        --key)
            [ $# -ge 2 ] || die "--key requires an argument"
            key="$2"
            shift 2
            ;;
        -*)
            die "Unknown option: $1"
            ;;
        *)
            if [ -n "$capsule" ]; then
                die "Multiple input capsules specified"
            fi
            capsule="$1"
            shift
            ;;
    esac
done

if $gen_cert && { [ -n "$cert" ] || [ -n "$key" ]; }; then
    die "--gen-cert cannot be combined with --cert or --key"
fi

if [ -z "$capsule" ]; then
    usage
fi

if [ ! -f "$capsule" ]; then
    die "File $capsule not found"
fi

if [ ! -f .config ]; then
    die "No '.config' file in current directory"
fi

command -v gcab &> /dev/null || die "gcab not found (install with: apt install gcab / dnf install gcab)"

# import coreboot's config file replacing $(...) with ${...}
while read -r line; do
    if ! eval "$line"; then
        die "failed to source '.config'"
    fi
done <<< "$(sed 's/\$(\([^)]\+\))/${\1}/g' .config)"

if [ "$CONFIG_DRIVERS_EFI_UPDATE_CAPSULES" != y ]; then
    die "Current board configuration lacks support of update capsules"
fi

date=$(stat -c %w "$capsule" | cut -d ' ' -f 1)
vendor=$(cat .config | grep -e "CONFIG_VENDOR_.*=y" | cut -d '=' -f 1 | cut -d '_' -f 3- | awk '{ print tolower($0) }')
version=$(echo $CONFIG_LOCALVERSION | tr -d 'v' | cut -d '-' -f 1)

archive_dir=$(mktemp --tmpdir -d XXXXXXXX)

# --- firmware.metainfo.xml (LVFS) ---

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

cp "$capsule" "${archive_dir}/firmware.bin"

# --- firmware.inf (Windows Update) ---

# Windows date format: MM/DD/YYYY
win_date=$(echo "$date" | awk -F- '{printf "%02d/%02d/%s", $2, $3, $1}')

# Version string as Major.Minor.Patch.0 for DriverVer
ver_major=$(echo "$version" | cut -d '.' -f 1)
ver_minor=$(echo "$version" | cut -d '.' -f 2)
ver_patch=$(echo "$version" | cut -d '.' -f 3)
ver_patch=${ver_patch:-0}
win_version="${ver_major}.${ver_minor}.${ver_patch}.0"

# GUID in uppercase for INF hardware ID and FirmwareId
guid_upper=$(echo "$CONFIG_DRIVERS_EFI_MAIN_FW_GUID" | tr '[:lower:]' '[:upper:]')

# Manufacturer display name: prefer SMBIOS manufacturer, fall back to vendor
if [ -n "$CONFIG_MAINBOARD_SMBIOS_MANUFACTURER" ]; then
    mfg_name="$CONFIG_MAINBOARD_SMBIOS_MANUFACTURER"
else
    mfg_name="${vendor^}"
fi

# firmware.bin is placed at %windir%\Firmware\{GUID}\ so that winload.efi
# can locate it at boot time before the driver store is accessible.
cat > "${archive_dir}/firmware.inf" << EOF
; Windows UEFI Firmware Update INF
; Generated by capsule_cabinet.sh for ${CONFIG_MAINBOARD_SMBIOS_PRODUCT_NAME} ${CONFIG_MAINBOARD_VERSION}

[Version]
Signature   = "\$WINDOWS NT\$"
Class       = Firmware
ClassGuid   = {f2e7dd72-6468-4e36-b6f1-6488f42c1b52}
Provider    = %Provider%
DriverVer   = ${win_date},${win_version}
PnpLockdown = 1
CatalogFile = firmware.cat

[Manufacturer]
%MfgName% = Firmware,NTamd64.10.0...17134

[Firmware.NTamd64.10.0...17134]
%FirmwareDesc% = Firmware_Install,UEFI\RES_{${guid_upper}}

[Firmware_Install.NT]
CopyFiles = Firmware_CopyFiles

[Firmware_CopyFiles]
firmware.bin

[Firmware_Install.NT.Hw]
AddReg = Firmware_AddReg

[Firmware_AddReg]
HKR,,FirmwareId,,{${guid_upper}}
HKR,,FirmwareVersion,%REG_DWORD%,${CONFIG_DRIVERS_EFI_MAIN_FW_VERSION}
HKR,,FirmwareFilename,,{${guid_upper}}\firmware.bin

[SourceDisksNames]
1 = %DiskName%

[SourceDisksFiles]
firmware.bin = 1

[DestinationDirs]
DefaultDestDir = 10,Firmware\{${guid_upper}}

[Strings]
Provider     = "${mfg_name}"
MfgName      = "${mfg_name}"
FirmwareDesc = "${CONFIG_MAINBOARD_SMBIOS_PRODUCT_NAME} ${CONFIG_MAINBOARD_VERSION} Firmware Update"
DiskName     = "Firmware Update"
REG_DWORD    = 0x00010001
EOF

# --- Windows Security Catalog (.cat) ---
# Generate the catalog from the files directly (before gcab) so the hashes
# are computed from the exact bytes that will go into the cabinet.

script_dir="$(dirname "$(realpath "$0")")"

# Optionally generate a self-signed code-signing certificate chain
if $gen_cert; then
    cert_base=$(echo "${CONFIG_MAINBOARD_SMBIOS_PRODUCT_NAME}" | tr '[:upper:]' '[:lower:]' | tr ' ' '-')
    cert="${cert_base}-signing.crt"
    key="${cert_base}-signing.key"
    echo "Generating test code-signing certificate chain..."
    python3 "${script_dir}/capsule_catalog.py" --gen-cert \
        --cert "$cert" --key "$key" \
        --subject "/CN=${CONFIG_MAINBOARD_SMBIOS_PRODUCT_NAME} Firmware Signing/O=${mfg_name}"
    ca_cert="${cert_base}-signing-ca.crt"
    echo "Chain cert (leaf+CA): $cert"
    echo "Leaf private key:     $key"
    echo "Root CA cert:         $ca_cert"
    echo ""
    echo "WARNING: Self-signed certificate chain — for test signing only."
    echo "  On the target Windows machine (run as Administrator):"
    echo "    bcdedit /set testsigning on"
    echo "    certutil -addstore Root \"${ca_cert}\""
    echo "    certutil -addstore TrustedPublisher \"${ca_cert}\""
fi

if [ -n "$cert" ] && [ -n "$key" ]; then
    if [ ! -f "$cert" ]; then
        die "Certificate file not found: $cert"
    fi
    if [ ! -f "$key" ]; then
        die "Key file not found: $key"
    fi
    if [ ! -f "${script_dir}/capsule_catalog.py" ]; then
        die "capsule_catalog.py not found alongside this script"
    fi

    # The catalog must be named "firmware.cat" to match the CatalogFile
    # directive in firmware.inf.  Hash the files directly so the catalog
    # contains exactly the same bytes that gcab will store in the cabinet.
    python3 "${script_dir}/capsule_catalog.py" \
        --cert "$cert" --key "$key" \
        --hwid "UEFI\\RES_{${CONFIG_DRIVERS_EFI_MAIN_FW_GUID^^}}" \
        --out "${archive_dir}/firmware.cat" \
        "${archive_dir}/firmware.bin" \
        "${archive_dir}/firmware.inf"
    win_cat="${capsule%.cap}.cat"
    cp "${archive_dir}/firmware.cat" "./${win_cat}"
    echo "File ${win_cat} created"
    echo "Note: For Windows Update, submit to Microsoft Hardware Dev Center or"
    echo "      sign with a Microsoft-trusted EV code-signing certificate."
else
    echo "Note: Provide --cert and --key to generate a signed .cat file."
    echo "      See capsule_catalog.py --help for details."
fi

# --- Build the cabinet ---

cab="${capsule%.cap}.cab"
pushd "$archive_dir" &> /dev/null
if [ -f firmware.cat ]; then
    gcab -c "$cab" firmware.bin firmware.metainfo.xml firmware.inf firmware.cat
else
    gcab -c "$cab" firmware.bin firmware.metainfo.xml firmware.inf
fi
popd &> /dev/null

cp "${archive_dir}/${cab}" ./
echo "File ${cab} created"

rm -r "$archive_dir"
