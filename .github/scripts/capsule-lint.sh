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
        ver="$ver-rc$((rc))"
    fi
    echo "$ver"
}

if [ ! -d configs ]; then
    echo "error: $(basename "$0") should be run from the root of coreboot's tree"
    exit 1
fi

declare -A guids
fail=0

mapfile -t configs < <(grep -l 'EFI_UPDATE_CAPSULE' configs/config.*)
for c in "${configs[@]}"; do
    local_ver=$(get_str "$c" 'CONFIG_LOCALVERSION')

    guid=$(get_str "$c" 'CONFIG_DRIVERS_EFI_MAIN_FW_GUID')
    ver=$(get_hex "$c" 'CONFIG_DRIVERS_EFI_MAIN_FW_VERSION')
    lsv=$(get_hex "$c" 'CONFIG_DRIVERS_EFI_MAIN_FW_LSV')

    # convert to lower case as normalization
    guid=${guid,,*}
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
