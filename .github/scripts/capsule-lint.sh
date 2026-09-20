#!/bin/bash

function get_str() {
    local cfg=$1
    local name=$2
    sed -n 's/^'"$name"'="\(.*\)"$/\1/p' "$cfg"
}

function get_hex() {
    local cfg=$1
    local name=$2
    sed -n 's/^'"$name"'=\(.*\)$/\1/p' "$cfg"
}

function hex_to_ver_str() {
    local hex=$1

    local major=${hex:2:2}
    local minor=${hex:4:2}
    local bugfix=${hex:6:2}
    local rc=${hex:8:2}

    local ver="v$((0x$major)).$((0x$minor)).$((0x$bugfix))"
    if [ "$((0x$rc))" -lt 128 ]; then
        ver="$ver-rc$((0x$rc))"
    fi
    echo "$ver"
}

if [ ! -d configs ]; then
    echo "error: $(basename "$0") should be run from the root of coreboot's tree"
    exit 1
fi

# one array for both main and EC capsule GUIDs, because the same value can't be
# reused regardless of what's being updated
declare -A guids
fail=0

mapfile -t configs < <(grep -l 'EFI_UPDATE_CAPSULE' configs/config.*)
for c in "${configs[@]}"; do
    local_ver=$(get_str "$c" 'CONFIG_LOCALVERSION')

    guid=$(get_str "$c" 'CONFIG_DRIVERS_EFI_MAIN_FW_GUID')
    ver=$(get_hex "$c" 'CONFIG_DRIVERS_EFI_MAIN_FW_VERSION')
    lsv=$(get_hex "$c" 'CONFIG_DRIVERS_EFI_MAIN_FW_LSV')

    # for EC capsules:
    #  - the version is not known at build time
    #  - LSV defaults to zero and there is no version to check it against
    ec_guid=$(get_str "$c" 'CONFIG_DRIVERS_EFI_EC_FW_GUID')
    ec_size=$(get_hex "$c" 'CONFIG_DRIVERS_EFI_EC_FW_SIZE')

    # convert to lower case as normalization
    guid=${guid,,*}
    ec_guid=${ec_guid,,*}
    # remove common prefix to shorten error messages
    c=${c##*/}

    if [ -z "$guid" ]; then
        echo "$c: missing CONFIG_DRIVERS_EFI_MAIN_FW_GUID"
        fail=1
    elif [ -n "${guids["$guid"]}" ]; then
        echo "$c: duplicated CONFIG_DRIVERS_EFI_MAIN_FW_GUID value ($guid)"
        echo "${c//?/ }  also appears in ${guids["$guid"]}"
        fail=1
    else
        guids["$guid"]=$c
    fi

    # EC capsules are optional
    if [ -n "$ec_guid" ]; then
        if [ -n "${guids["$ec_guid"]}" ]; then
            echo "$c: duplicated CONFIG_DRIVERS_EFI_EC_FW_GUID value ($ec_guid)"
            echo "${c//?/ }  also appears in ${guids["$ec_guid"]}"
            fail=1
        else
            guids["$ec_guid"]=$c
        fi

        if [ -z "$ec_size" ] || [ $(( $ec_size )) -le 0 ]; then
            echo "$c: invalid CONFIG_DRIVERS_EFI_EC_FW_SIZE value ($ec_size)"
            fail=1
        fi
    fi

    if [ -z "$ver" ]; then
        echo "$c: missing CONFIG_DRIVERS_EFI_MAIN_FW_VERSION"
        fail=1
    fi
    if [ -z "$lsv" ]; then
        echo "$c: missing CONFIG_DRIVERS_EFI_MAIN_FW_LSV"
        fail=1
    fi

    if [ -n "$ver" ] && [ -n "$lsv" ] && [ $(( ver < lsv )) -eq 1 ]; then
        echo "$c: CONFIG_DRIVERS_EFI_MAIN_FW_VERSION < CONFIG_DRIVERS_EFI_MAIN_FW_LSV"
        echo "${c//?/ }  $ver < $lsv"
        fail=1
    fi

    if [ -n "$ver" ]; then
        str_ver=$(hex_to_ver_str "$ver")
        if [ "$str_ver" != "$local_ver" ]; then
            echo "$c: CONFIG_LOCALVERSION doesn't match CONFIG_DRIVERS_EFI_MAIN_FW_VERSION"
            echo "${c//?/ }  $local_ver != $str_ver ($ver)"
            fail=1
        fi
    fi
done

exit "$fail"
