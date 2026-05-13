#!/bin/bash

set -e

edk_workspace=payloads/external/edk2/workspace
edk_basetools=${edk_workspace}/Dasharo/BaseTools
edk_tools=${edk_basetools}/BinWrappers/PosixLike
edk_scripts=${edk_basetools}/Scripts

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
    echo '  box     export standalone GenerateCapsule out of EDK2'
    echo '  help    print this message'
    echo '  keygen  use OpenSSL to auto-generate test keys suitable for signing'
    echo '          positional argument: directory-path'
    echo '  make    build a capsule, options:'
    echo '          -t root-certificate-file'
    echo '          -o subroot-certificate-file'
    echo '          -s signing-certificate-file'
    echo '          -b (the flag adds battery check DXE into the capsule)'
}

function help_subcommand() {
    print_usage
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
    if [ ! -x "${edk_tools}/GenerateCapsule" ]; then
        die "'${edk_tools}/GenerateCapsule' can't be executed"
    fi

    # import coreboot's config file replacing $(...) with ${...}
    while read -r line; do
        if ! eval "$line"; then
            die "failed to source '.config'"
        fi
    done <<< "$(sed 's/\$(\([^)]\+\))/${\1}/g' .config)"

    if [ "$CONFIG_DRIVERS_EFI_UPDATE_CAPSULES" != y ]; then
        die "current board configuration lacks support of update capsules"
    fi

    # Option names match terminology of GenerateCapsule which conveniently start
    # with different letters:
    #  * t - trusted
    #  * o - other
    #  * s - signer
    local root_cert sub_cert sign_cert include_battery_check
    while getopts "t:o:s:b" OPTION; do
        case $OPTION in
            t) root_cert="$OPTARG" ;;
            o) sub_cert="$OPTARG" ;;
            s) sign_cert="$OPTARG" ;;
            b) include_battery_check=1 ;;
            *) exit 1 ;;
        esac
    done

    check_cert root "$root_cert"
    check_cert sub "$sub_cert"
    check_cert sign "$sign_cert"

    local cap_file=${CONFIG_MAINBOARD_DIR//[\/-]/_}
    if [[ ${CONFIG_MAINBOARD_PART_NUMBER} =~ DDR4 ]]; then
        cap_file+=_ddr4
    fi
    cap_file+=_${CONFIG_LOCALVERSION}
    cap_file+=.cap

    local cap_flags="--capflag PersistAcrossReset"
    # Capsules on AMD boards do not survive resets
    if [ "$CONFIG_EDK2_CAPSULE_DOES_NOT_SURVIVE_RESET" == y ]; then
        cap_flags=""
    fi

    if [ -e "$cap_file" ]; then
        confirm "Overwrite already existing '$cap_file'?"
    fi

    local build_type
    if [ "$CONFIG_EDK2_RELEASE" = y ]; then
        build_type=RELEASE
    else
        build_type=DEBUG
    fi

    local json_file
    json_file=$(mktemp --tmpdir --suffix -cap.json XXXXXXXX)
    trap "$(printf 'rm -f %q' "$json_file")" EXIT

    cat > "$json_file" << EOF
{
    "EmbeddedDrivers": [
EOF

    # Ensure the charger check driver module is first
    if [ "$include_battery_check" = 1 ]; then
      cat >> "$json_file" << EOF
        {
            "Driver": "${edk_workspace}/Build/DasharoPayloadPkgX64/${build_type}_GCC/X64/CapsuleChargerCheckDxe.efi"
        },
EOF
    fi

    cat >> "$json_file" << EOF
        {
            "Driver": "${edk_workspace}/Build/DasharoPayloadPkgX64/${build_type}_GCC/X64/CapsuleSplashDxe.efi"
        },
        {
            "Driver": "${edk_workspace}/Build/DasharoPayloadPkgX64/${build_type}_GCC/X64/FmpDxe.efi"
        }
    ],
    "Payloads": [
        {
            "Payload": "build/coreboot.rom",
            "Guid": "${CONFIG_DRIVERS_EFI_MAIN_FW_GUID}",
            "FwVersion": "${CONFIG_DRIVERS_EFI_MAIN_FW_VERSION}",
            "LowestSupportedVersion": "${CONFIG_DRIVERS_EFI_MAIN_FW_LSV}",

            "OpenSslSignerPrivateCertFile": "${sign_cert}",
            "OpenSslOtherPublicCertFile": "${sub_cert}",
            "OpenSslTrustedPublicCertFile": "${root_cert}"
        }
    ]
}
EOF

    # Linux doesn't support InitiateReset flag, omitting it to rely on manual
    # warm reset
    if ! "${edk_tools}/GenerateCapsule" --encode \
                                        $cap_flags \
                                        --json-file "$json_file" \
                                        --output "$cap_file"; then
        die "GenerateCapsule failed"
    fi

    echo "Created the capsule at '$cap_file'"
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

if [ $# -eq 0 ]; then
    print_usage
    exit 1
fi

subcommand=$1
shift

case "$subcommand" in
    box|help|keygen|make)
        "$subcommand"_subcommand "$@" ;;

    *)
        echo "Unexpected subcommand: $subcommand"
        echo
        print_usage
        exit 1 ;;
esac
