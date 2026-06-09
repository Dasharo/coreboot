#!/bin/bash

set -e

edk_workspace=payloads/external/edk2/workspace
edk_basetools=${edk_workspace}/Dasharo/BaseTools
edk_tools=${edk_basetools}/BinWrappers/PosixLike
edk_scripts=${edk_basetools}/Scripts
# gets overwritten with gencap/GenerateCapsule if it exists and is executable,
# that directory is created by `box` subcommand and allows not depending on
# EDK2's repository for resigning
generate_capsule=${edk_tools}/GenerateCapsule

function die() {
    echo error: "$@" 1>&2
    exit 1
}

function confirm() {
    local msg=$1
    read -r -n1 -p "$msg [y/N] "
    echo
    if [ "$REPLY" != y ]; then
        die "operation cancelled by the user"
    fi
}

function print_banner() {
    local msg=$1

    echo
    echo ========== "${msg^^*}" ==========
    echo
}

function info() {
    echo info: "$@"
}

function print_usage() {
    echo "Usage: $(basename "$0") subcommand [subcommand-args...]"
    echo
    echo 'Subcommands:'
    echo '  box            export standalone GenerateCapsule out of EDK2'
    echo '  help           print this message'
    echo '  keygen         use OpenSSL to auto-generate test keys suitable for signing'
    echo '                 positional argument: directory-path'
    echo '  make           build a capsule, options:'
    echo '                 -t root-certificate-file'
    echo '                 -o subroot-certificate-file'
    echo '                 -s signing-certificate-file'
    echo '                 [-b] (include battery check DXE in the capsule)'
    echo '                 [-y] (overwrite destination without prompting)'
    echo '                 [-e ec-rom-file] (make an EC firmware capsule)'
    echo '                 [-c cap-file] (destination file name, generated if omitted)'
    echo '  resign         resign an existing capsule with a different key'
    echo '                 -t root-certificate-file'
    echo '                 -o subroot-certificate-file'
    echo '                 -s signing-certificate-file'
    echo '                 positional arguments: input-capsule output-capsule'
    echo '  create_cabinet create a fwupd cabinet (.cab) from a capsule'
    echo '                 positional argument: capsule-file'
    echo '  upload_lvfs    upload a cabinet (.cab) to LVFS, options'
    echo '                 [-c credentials-file] (defaults to ~/.config/dasharo-credentials/lvfs)'
    echo '                 [-u lvfs-base-url]    (defaults to $LVFS_URL)'
    echo '                 [-e email]            (defaults to $LVFS_EMAIL)'
    echo '                 [-t token]            (defaults to $LVFS_TOKEN)'
    echo '                 positional argument:'
    echo '                 cabinet-file (optional if exactly one .cab in current dir)'
}

function help_subcommand() {
    print_usage
}

function source_coreboot_config() {
    local config_file=${1-.config}

    if [ ! -f "$config_file" ]; then
            die "config file '$config_file' doesn't exist"
    fi

    local line

    while read -r line; do
        if ! eval "$line"; then
            die "failed to source '.config'"
        fi
    done <<< "$(sed 's/\$(\([^)]\+\))/${\1}/g' $config_file)"
}

function require_capsule_support() {
    if [ "$CONFIG_DRIVERS_EFI_UPDATE_CAPSULES" != y ]; then
        die "current board configuration lacks support of update capsules"
    fi
}

function keygen_subcommand() {
    local dir=$1

    if [ $# -ne 1 ]; then
        echo "Usage: $(basename "$0") keygen keys-dir"
        exit 1
    fi

    if [ -e "$dir" ]; then
        confirm "OK to remove '$dir'?"
        rm -r "$dir"
    fi

    mkdir "$dir"
    cd "$dir"

    # this is needed to make `openssl req` work non-interactively
    cat > "openssl.cnf" << 'EOF'
.include /etc/ssl/openssl.cnf

[ CA_default ]
dir           = ./test-ca
certs         = $dir/certs
crl_dir       = $dir/crl
database      = $dir/index.txt
new_certs_dir = $dir/newcerts
certificate   = $dir/cacert.pem
serial        = $dir/serial
crlnumber     = $dir/crlnumber
crl           = $dir/crl.pem
private_key   = $dir/private/cakey.pem

[ req_root ]
prompt             = no
distinguished_name = req_root_dn
x509_extensions    = v3_ca
string_mask        = utf8only
[ req_root_dn ]
countryName         = XX
stateOrProvinceName = Province
organizationName    = Org
commonName          = root

[ req_sub ]
prompt             = no
distinguished_name = req_sub_dn
x509_extensions    = v3_ca
string_mask        = utf8only
[ req_sub_dn ]
countryName         = XX
stateOrProvinceName = Province
organizationName    = Org
commonName          = sub

[ req_sign ]
prompt             = no
distinguished_name = req_sign_dn
x509_extensions    = v3_ca
string_mask        = utf8only
[ req_sign_dn ]
countryName         = XX
stateOrProvinceName = Province
organizationName    = Org
commonName          = sign
EOF

    print_banner 'Making root certificate'

    # make root certificate
    openssl genrsa -out root.p8e 2048
    openssl req -config openssl.cnf -section req_root -new -x509 -days 3650 -key root.p8e -out root.pub.pem

    # dump certificate information like `openssl ca` does for completeness
    openssl x509 -in root.pub.pem -text -certopt no_sigdump,no_pubkey -nocert

    # create a CA
    mkdir -p test-ca/newcerts
    touch test-ca/index.txt
    echo 01 > test-ca/serial

    openssl x509 -in root.pub.pem -out root.cer -outform DER
    python "${OLDPWD}/${edk_scripts}/BinToPcd.py" \
        -p gFmpDevicePkgTokenSpaceGuid.PcdFmpDevicePkcs7CertBufferXdr \
        -i root.cer -x -o CapsuleRootKey.inc

    print_banner 'Making subroot certificate'

    # make subroot certificate
    openssl genrsa -out sub.p8e 2048
    openssl req -config openssl.cnf -section req_sub -new -key sub.p8e -out sub.csr
    yes | openssl ca -config openssl.cnf -extensions v3_ca -in sub.csr -days 3650 -cert root.pub.pem -keyfile root.p8e -notext -out sub.pub.pem

    print_banner 'Making signing certificate'

    # make signing certificate
    openssl genrsa -out sign.p8e 2048
    openssl req -config openssl.cnf -section req_sign -new -key sign.p8e -out sign.csr
    yes | openssl ca -config openssl.cnf -in sign.csr -days 3650 -cert sub.pub.pem -keyfile sub.p8e -notext -out sign.crt

    # create binary PKCS12 (certificate + private key) from signing certificate
    openssl pkcs12 -export -passout pass: -inkey sign.p8e -in sign.crt -out sign.pfx
    # now convert binary PKCS12 into PEM PKCS12 with no password
    openssl pkcs12 -in sign.pfx -passin pass: -nodes -out sign.p12

    print_banner 'Usage examples'

    echo "Installing root certificate (before build):"
    echo "  cp $dir/CapsuleRootKey.inc ${edk_workspace}/Dasharo/DasharoPayloadPkg/"
    echo
    echo "Signing a capsule (after build):"
    echo "  $0 make -t $dir/root.pub.pem -o $dir/sub.pub.pem -s $dir/sign.p12"
}

function check_cert() {
    local name=$1
    local path=$2

    if [ -z "$path" ]; then
        die "$name certificate wasn't provided"
    fi

    if [ ! -f "$path" ]; then
        die "can't read $name certificate at '$path'"
    fi
}

function check_generate_capsule() {
    if [ -x gencap/GenerateCapsule ]; then
        info "found 'gencap/GenerateCapsule'"
        generate_capsule=gencap/GenerateCapsule
    fi

    if [ ! -x "$generate_capsule" ]; then
        die "'${generate_capsule}' can't be executed"
    fi
}

# This function assumes .config has been sourced.
function build_capsule() {
    local payload=$1
    local guid=$2
    local version=$3
    local lsv=$4
    local -n drivers=$5
    local -n certs=$6
    local cap_file=$7

    local cap_flags=( --capflag PersistAcrossReset )
    # Capsules on AMD boards do not survive resets
    if [ "$CONFIG_EDK2_CAPSULE_DOES_NOT_SURVIVE_RESET" = y ]; then
        cap_flags=()
    fi

    local v2_capsule=no
    if [ "${CONFIG_EDK2_CAPSULES_V2:-n}${CONFIG_EDK2_CAPSULES_V2_TRANSITION:-n}" = yn ]; then
        v2_capsule=yes
    fi

    local json_file
    json_file=$(mktemp --tmpdir --suffix -cap.json XXXXXXXX)
    trap "$(printf 'rm -f -- %q %q' "$json_file" "$cap_file.inner")" EXIT

    local opt_root_cert=${certs[root]}
    local opt_sub_cert=${certs[sub]}
    local opt_sign_cert=${certs[sign]}
    if [ "$v2_capsule" = yes ]; then
        # The inner capsule is always signed with the test key.  Not signing it
        # at all doesn't work because FmpDxe doesn't accept unsigned payloads at
        # least due to Image->AuthInfo.Hdr.wRevision check in
        # AuthenticateFmpImage().
        opt_root_cert=${edk_basetools}/Source/Python/Pkcs7Sign/TestRoot.pub.pem
        opt_sub_cert=${edk_basetools}/Source/Python/Pkcs7Sign/TestSub.pub.pem
        opt_sign_cert=${edk_basetools}/Source/Python/Pkcs7Sign/TestCert.pem
    fi

    cat > "$json_file" << EOF
{
    "EmbeddedDrivers": [
$(printf '        { "Driver": "%s" },\n' "${drivers[@]}" | sed '$s/,$//')
    ],
    "Payloads": [
        {
            "Payload": "${payload}",
            "Guid": "${guid}",
            "FwVersion": "${version}",
            "LowestSupportedVersion": "${lsv}",

            "OpenSslSignerPrivateCertFile": "${opt_sign_cert}",
            "OpenSslOtherPublicCertFile": "${opt_sub_cert}",
            "OpenSslTrustedPublicCertFile": "${opt_root_cert}"
        }
    ]
}
EOF

    if [ "$v2_capsule" = yes ]; then
        # The capsule created above is the inner capsule.  Make it and then
        # update JSON file to point at it as a payload.
        if ! "$generate_capsule" --encode \
                                 "${cap_flags[@]}" \
                                 --json-file "$json_file" \
                                 --output "$cap_file.inner"; then
            die "GenerateCapsule failed"
        fi

        # The outer capsule is signed with the key passed by the user.
        cat > "$json_file" << EOF
{
    "EmbeddedDrivers": [],
    "Payloads": [
        {
            "Payload": "$cap_file.inner",
            "Guid": "${guid}",
            "FwVersion": "${version}",
            "LowestSupportedVersion": "${lsv}",

            "OpenSslSignerPrivateCertFile": "${certs[sign]}",
            "OpenSslOtherPublicCertFile": "${certs[sub]}",
            "OpenSslTrustedPublicCertFile": "${certs[root]}"
        }
    ]
}
EOF
    fi

    # Linux doesn't support InitiateReset flag, omitting it to rely on manual
    # warm reset
    if ! "$generate_capsule" --encode \
                             "${cap_flags[@]}" \
                             --json-file "$json_file" \
                             --output "$cap_file"; then
        die "GenerateCapsule failed"
    fi
}

# Prints version of an EC ROM derived from the file or dies.
# The implementation of version conversion must be kept in sync with
# parse_ec_version() in src/drivers/efi/info.c
function extract_ec_version() {
    local ec_rom_file=$1

    local ver
    ver=$(strings "$ec_rom_file" | \
          sed -n '/^76EC_VERSION/s/.*=\([0-9]\{4\}\(-[0-9][0-9]\)\{2\}\).*/\1/p' | \
          head -1)

    if [ -z "$ver" ]; then
        die "Failed to extract 76EC_VERSION from '$ec_rom_file'"
    fi

    local y=${ver::4}
    local m=${ver:5:2}
    local d=${ver:8:2}

    echo $(( ((10#$y & 0xffff) << 16) | ((10#$m & 0xff) << 8) | (10#$d & 0xff) ))
}

function make_subcommand() {
    if [ ! -f .config ]; then
        die "no '.config' file in current directory"
    fi
    if [ ! -f build/coreboot.rom ]; then
        die "no 'build/coreboot.rom'; the firmware wasn't built?"
    fi
    if [ ! build/coreboot.rom -nt .config ]; then
        die "'build/coreboot.rom' is not newer than .config'; need a re-build?"
    fi

    check_generate_capsule

    source_coreboot_config
    require_capsule_support

    # Option names for key files match terminology of GenerateCapsule which,
    # conveniently, has words starting with different letters:
    #  * t - trusted
    #  * o - other
    #  * s - signer

    local -A cap_certs
    local include_battery_check overwrite_output ec_rom_file cap_file
    while getopts "t:o:s:be:yc:" OPTION; do
        case $OPTION in
            t) cap_certs[root]="$OPTARG" ;;
            o) cap_certs[sub]="$OPTARG" ;;
            s) cap_certs[sign]="$OPTARG" ;;
            b) include_battery_check=1 ;;
            y) overwrite_output=1 ;;
            e) ec_rom_file="$OPTARG" ;;
            c) cap_file="$OPTARG" ;;
            *) exit 1 ;;
        esac
    done

    check_cert root "${cap_certs[root]}"
    check_cert sub "${cap_certs[sub]}"
    check_cert sign "${cap_certs[sign]}"

    # Assuming a coreboot capsule at first.
    local rom_file=build/coreboot.rom
    local guid=$CONFIG_DRIVERS_EFI_MAIN_FW_GUID
    local splash_guid=E1CBE3CC-3D32-44CF-8DB9-2A78BA16F2F6
    local version=$CONFIG_DRIVERS_EFI_MAIN_FW_VERSION
    local lsv=$CONFIG_DRIVERS_EFI_MAIN_FW_LSV

    if [ -n "$ec_rom_file" ]; then
        if [ -z "$CONFIG_DRIVERS_EFI_EC_FW_GUID" ]; then
            die '-e is passed but CONFIG_DRIVERS_EFI_EC_FW_GUID is empty'
        fi

        assert_file_exists "$ec_rom_file"

        local ec_rom_version
        ec_rom_version=$(extract_ec_version "$ec_rom_file")

        local size
        size=$(stat --printf %s "$ec_rom_file")
        if [ "$size" -ne $(( $CONFIG_DRIVERS_EFI_EC_FW_SIZE )) ]; then
            die "$(printf "'%s' is 0x%x bytes in size instead of 0x%x" \
                          "$ec_rom_file" \
                          "$size" \
                          "$CONFIG_DRIVERS_EFI_EC_FW_SIZE")"
        fi

        rom_file=$ec_rom_file
        guid=$CONFIG_DRIVERS_EFI_EC_FW_GUID
        splash_guid=8C675702-A60D-462C-B950-12F00FFDFFF3
        version=$ec_rom_version
        lsv=$CONFIG_DRIVERS_EFI_EC_FW_LSV
    fi

    if [ -z "$cap_file" ]; then
        cap_file=${CONFIG_MAINBOARD_DIR//[\/-]/_}
        if [[ ${CONFIG_MAINBOARD_PART_NUMBER} =~ DDR4 ]]; then
            cap_file+=_ddr4
        fi
        if [ -n "$ec_rom_file" ]; then
            cap_file+=_ec
        fi
        cap_file+=_${CONFIG_LOCALVERSION}
        cap_file+=.cap
    fi

    if [ "$overwrite_output" != 1 ] && [ -e "$cap_file" ]; then
        confirm "Overwrite already existing '$cap_file'?"
    fi

    local build_type
    if [ "$CONFIG_EDK2_RELEASE" = y ]; then
        build_type=RELEASE
    else
        build_type=DEBUG
    fi

    local build_dir=${edk_workspace}/Build/DasharoPayloadPkgX64/${build_type}_GCC/X64

    # Ensure the charger check driver module is first
    local embedded_drivers=()
    if [ "$include_battery_check" = 1 ]; then
        embedded_drivers+=( "${build_dir}/CapsuleChargerCheckDxe.efi" )
    fi
    embedded_drivers+=(
        # Can't use the same GUID more than once and can't easily derive it
        # either, so CapsuleSplashDxe has hard-coded GUIDs for main/EC firmware
        # and one of them is selected above.
        "${build_dir}/DasharoPayloadPkg/CapsuleSplashDxe/${splash_guid}/OUTPUT/CapsuleSplashDxe.efi"
        "${build_dir}/FmpDevicePkg/FmpDxe/${guid}/OUTPUT/FmpDxe.efi"
    )

    build_capsule "$rom_file" "$guid" "$version" "$lsv" \
                  embedded_drivers cap_certs \
                  "$cap_file"

    echo "Created the capsule at '$cap_file'"
}

function assert_file_exists() {
    local path=$1

    if [ ! -f "$path" ]; then
        die "File '$path' not found"
    fi
}

function assert_file_is_a_capsule() {
    local path=$1

    local fmp_guid_bytes_hex=edd5cb6d2de8444cbda17194199ad92a
    if [ "$(xxd -l 16 -ps "$path")" != "$fmp_guid_bytes_hex" ]; then
        die "'$path' is not an FMP capsule file"
    fi
}

function assert_command_exists() {
    local cmd=$1

    if ! command -v "$cmd" >/dev/null 2>&1; then
        die "'$cmd' not found in PATH"
    fi
}

function decode_capsule() {
    local tmp_dir=$1
    local capsule=$2
    local -n result=$3

    "$generate_capsule" --decode "$capsule" \
                        --output "$tmp_dir/decoded"

    local json_file="$tmp_dir/decoded.json"
    result["fw_version"]=$(jq -r '.Payloads[0].FwVersion' "$json_file")
    result["guid"]=$(jq -r '.Payloads[0].Guid' "$json_file")
    result["lowest_supported_version"]=$(jq -r '.Payloads[0].LowestSupportedVersion' "$json_file")
    result["payload"]=$(jq -r '.Payloads[0].Payload' "$json_file")

    local drivers
    drivers=$(ls -v "$tmp_dir"/decoded.EmbeddedDriver* 2>/dev/null | while read -r f; do
        printf '    { "Driver": "%s" },\n' "$f"
    done | sed '$s/,$//')
    result["drivers"]="$drivers"
}

function encode_capsule() {
    local tmp_dir=$1
    local cap_out=$2
    local -n json_data=$3

    cat > "$tmp_dir/cap.json" <<EOF
{
  "EmbeddedDrivers": [
${json_data["drivers"]}
  ],
  "Payloads": [
    {
      "Payload": "${json_data["payload"]}",
      "Guid": "${json_data["guid"]}",
      "FwVersion": "${json_data["fw_version"]}",
      "LowestSupportedVersion": "${json_data["lowest_supported_version"]}",
      "OpenSslSignerPrivateCertFile": "${json_data["sign_cert"]}",
      "OpenSslOtherPublicCertFile": "${json_data["sub_cert"]}",
      "OpenSslTrustedPublicCertFile": "${json_data["root_cert"]}"
    }
  ]
}
EOF

    local cap_flags=( --capflag PersistAcrossReset )
    # Capsules on AMD boards do not survive resets
    if [ "$CONFIG_EDK2_CAPSULE_DOES_NOT_SURVIVE_RESET" = y ]; then
        cap_flags=()
    fi

    "$generate_capsule" --encode \
                        "${cap_flags[@]}" \
                        --json-file "$tmp_dir/cap.json" \
                        --output "$cap_out"
}

function resign_subcommand() {
    check_generate_capsule

    local root_cert sub_cert sign_cert
    OPTIND=1
    while getopts "t:o:s:" OPTION; do
        case $OPTION in
            t) root_cert="$OPTARG" ;;
            o) sub_cert="$OPTARG" ;;
            s) sign_cert="$OPTARG" ;;
            *) exit 1 ;;
        esac
    done
    shift $((OPTIND - 1))

    if [ $# -ne 2 ]; then
        die "Incorrect number of positional parameters: $# (expected: 2)"
    fi

    local in_capsule=$1
    local out_capsule=$2

    assert_file_exists "$in_capsule"
    assert_file_is_a_capsule "$in_capsule"
    if [ -e "$out_capsule" ]; then
        confirm "Overwrite already existing '$out_capsule'?"
    fi
    assert_command_exists jq
    assert_command_exists cbfstool

    local tmp_dir
    tmp_dir=$(mktemp --tmpdir --directory --suffix -cap XXXXXXXX)
    trap "$(printf 'rm -r -- %q' "$tmp_dir")" EXIT

    check_cert root "$root_cert"
    check_cert sub "$sub_cert"
    check_cert sign "$sign_cert"

    local -A metadata
    decode_capsule "$tmp_dir" "$in_capsule" metadata
    metadata[sign_cert]=$sign_cert
    metadata[sub_cert]=$sub_cert
    metadata[root_cert]=$root_cert

    local -A metadata_inner
    mkdir "$tmp_dir/inner"
    decode_capsule "$tmp_dir/inner" "${metadata[payload]}" metadata_inner
    cbfstool "${metadata_inner[payload]}" extract -f "$tmp_dir/inner/.config" -n config 2>/dev/null
    source_coreboot_config "$tmp_dir/inner/.config"

    encode_capsule "$tmp_dir" "$out_capsule" metadata
}

function create_cabinet_subcommand() {
    if [ $# -ne 1 ]; then
        die "Incorrect number of input parameters specified: $# (expected: 1)"
    fi

    local capsule=$1
    local cabinet="${capsule%.cap}.cab"

    if [ -z "$capsule" ]; then
        die "No input capsule specified"
    fi

    assert_file_exists "$capsule"
    assert_file_is_a_capsule "$capsule"
    assert_command_exists fwupdtool

    source_coreboot_config
    require_capsule_support

    local date vendor version
    date=$(stat -c %w "$capsule" 2>/dev/null | cut -d ' ' -f 1)
    if [ -z "$date" ] || [ "$date" = "-" ]; then
        date=$(stat -c %y "$capsule" 2>/dev/null | cut -d ' ' -f 1)
    fi

    vendor=$(grep -e "CONFIG_VENDOR_.*=y" .config | cut -d '=' -f 1 | cut -d '_' -f 3- | awk '{ print tolower($0) }')
    version=$(echo "$CONFIG_LOCALVERSION" | tr -d 'v' | cut -d '-' -f 1)

    local archive_dir
    archive_dir=$(mktemp --tmpdir -d XXXXXXXX)
    trap 'rm -rf -- "$archive_dir"' EXIT

    local id=com.${vendor}.${CONFIG_MAINBOARD_SMBIOS_PRODUCT_NAME}.${CONFIG_MAINBOARD_VERSION}.system.firmware
    id=${id// /_}
    id=${id////_}

    cat > "${archive_dir}/firmware.metainfo.xml" << EOF
<?xml version='1.0' encoding='utf-8'?>
<component type="firmware">
  <id>${id}</id>
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

    pushd "$archive_dir" >/dev/null
    fwupdtool build-cabinet "${cabinet}" firmware.bin firmware.metainfo.xml
    popd >/dev/null

    cp "${archive_dir}/${cabinet}" ./

    echo "File ${cabinet} created"
}

function box_subcommand() {
    local src=${edk_basetools}/Source/Python
    local dst=gencap

    local keys=${src}/Pkcs7Sign

    if [ -e "$dst" ]; then
        confirm "Overwrite already existing '$dst'?"
        rm -r "${dst}"
    fi

    info "using '${src}'"
    info "constructing a standalone version in '${dst}'"

    mkdir -p "${dst}"/{Common,keys}

    cp "${src}/Capsule/GenerateCapsule.py" "${dst}"
    cp -r "${src}/Common/Edk2" "${dst}/Common"
    cp -r "${src}/Common/Uefi" "${dst}/Common"

    info "using keys from '${keys}'"
    cp "$keys"/TestRoot.pub.pem "${dst}/keys/root.pub.pem"
    cp "$keys"/TestSub.pub.pem "${dst}/keys/sub.pub.pem"
    cp "$keys"/TestCert.pem "${dst}/keys/sign.crt"

    cat > "${dst}/GenerateCapsule" <<'EOF'
#!/usr/bin/env bash

python=python
if command -v python3 >/dev/null; then
    python=python3
elif command -v python2 >/dev/null; then
    python=python2
fi

dir=$(dirname "${BASH_SOURCE:-$0}")
exec "${python}" "${dir}/GenerateCapsule.py" "$@"
EOF
    chmod +x "${dst}/GenerateCapsule"

    print_banner 'Help'
    echo "Location of capsule signing keys:"
    echo "  ${dst}/keys/"
    echo
    echo "Usage examples:"
    echo "  ${dst}/GenerateCapsule --help"
    echo "  ${dst}/GenerateCapsule --output decoded --decode coreboot.cap"
}

function upload_lvfs_subcommand() {
    local default_creds_file creds_file opt_creds_file opt_url opt_email opt_token
    local env_email env_token env_url
    local email token base_url cabinet
    local url response status body curl_rc

    default_creds_file="${XDG_CONFIG_HOME:-$HOME/.config}/dasharo-credentials/lvfs"
    creds_file="$default_creds_file"

    opt_creds_file=""
    opt_url=""
    opt_email=""
    opt_token=""

    OPTIND=1
    while getopts "c:u:e:t:" OPTION; do
        case $OPTION in
            c) opt_creds_file="$OPTARG" ;;
            u) opt_url="$OPTARG" ;;
            e) opt_email="$OPTARG" ;;
            t) opt_token="$OPTARG" ;;
            *) exit 1 ;;
        esac
    done
    shift $((OPTIND - 1))

    cabinet="$1"

    if [ -n "$opt_creds_file" ]; then
        creds_file="$opt_creds_file"
    fi

    env_email="${LVFS_EMAIL-}"
    env_token="${LVFS_TOKEN-}"
    env_url="${LVFS_URL-}"

    if [ -r "$creds_file" ]; then
        # shellcheck disable=SC1090
        . "$creds_file"
    fi

    if [ -n "$env_email" ]; then
        LVFS_EMAIL="$env_email"
    fi
    if [ -n "$env_token" ]; then
        LVFS_TOKEN="$env_token"
    fi
    if [ -n "$env_url" ]; then
        LVFS_URL="$env_url"
    fi

    email="${LVFS_EMAIL-}"
    token="${LVFS_TOKEN-}"
    base_url="${LVFS_URL-https://fwupd.org}"

    if [ -n "$opt_email" ]; then
        email="$opt_email"
    fi
    if [ -n "$opt_token" ]; then
        token="$opt_token"
    fi
    if [ -n "$opt_url" ]; then
        base_url="$opt_url"
    fi

    if [ -z "$cabinet" ]; then
        set -- ./*.cab
        if [ ! -e "$1" ]; then
            die "No cabinet specified and no .cab found in current directory"
        fi
        if [ $# -ne 1 ]; then
            die "Multiple .cab files found in current directory, specify the cabinet path explicitly"
        fi
        cabinet="$1"
    fi

    assert_file_exists "$cabinet"

    if [ -z "$email" ]; then
        die "LVFS email is not set. Put LVFS_EMAIL into '$creds_file' or pass -e"
    fi
    if [ -z "$token" ]; then
        die "LVFS token is not set. Put LVFS_TOKEN into '$creds_file' or pass -t"
    fi

    assert_command_exists curl

    url="${base_url%/}/lvfs/upload/token"

    set +e
    response=$(curl -sS -X POST \
        --connect-timeout 10 --max-time 300 \
        --retry 3 --retry-delay 2 --retry-connrefused \
        -F "file=@${cabinet}" \
        --user "${email}:${token}" \
        -w "\n%{http_code}" \
        "$url")
    curl_rc=$?
    set -e

    if [ "$curl_rc" -ne 0 ]; then
        die "curl failed with exit code $curl_rc"
    fi

    status="${response##*$'\n'}"
    body="${response%$'\n'*}"

    if [ "$status" -lt 200 ] || [ "$status" -ge 300 ]; then
        echo "$body" 1>&2
        die "LVFS upload failed with HTTP status $status"
    fi

    echo "$body"
}

if [ $# -eq 0 ]; then
    print_usage
    exit 1
fi

subcommand=$1
shift

case "$subcommand" in
    box|help|keygen|make|resign|create_cabinet|upload_lvfs)
        "$subcommand"_subcommand "$@" ;;

    *)
        echo "Unexpected subcommand: $subcommand"
        echo
        print_usage
        exit 1 ;;
esac
