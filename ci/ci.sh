#!/usr/bin/env bash

set -euo pipefail

script_dir="$(dirname "${BASH_SOURCE[0]}")"

vendor=""
model=""

usage() {
    echo "Usage: ci.sh -v vendor -m model -f firmware command" 1>&2
    exit 1
}

while getopts "r:v:m:f:" o; do
    case "${o}" in
        r) root="${OPTARG}" ;;
        v) vendor="${OPTARG}" ;;
        m) model="${OPTARG}" ;;
        f) fw_file="${OPTARG}" ;;
        *) usage ;;
    esac
done
shift $((OPTIND-1))

[ -n "$vendor" ] || usage
[ -n "$model" ] || usage
[ $# -eq 1 ] || usage

command="$1"
[[ "$command" =~ ^(flash|test)$ ]] || usage
shift 1

options=(
    -L TRACE
    -v snipeit:none
)

compatibility_tests=()
performance_tests=()
security_tests=()
stability_tests=()

case "$vendor" in
    msi)
        case "$model" in
            ms7d25_ddr4)
                device_ip="192.168.10.183"
                rte_ip="192.168.10.199"
                pikvm_ip="192.168.10.16"
                config="msi-pro-z690-a-ddr4"
                ;;
            ms7d25_ddr5)
                device_ip="192.168.10.93"
                rte_ip="192.168.10.188"
                pikvm_ip="192.168.10.45"
                config="msi-pro-z690-a-ddr5"
                ;;
            ms7e06_ddr5)
                device_ip="192.168.10.10"
                rte_ip="192.168.10.127"
                pikvm_ip="192.168.10.226"
                config="msi-pro-z790-p-ddr5"
                ;;
            *)
                echo unknown board: $model
                exit 1
                ;;
        esac
        ;;
    *)
        echo unknown vendor: $vendor
        exit 1
        ;;
esac

options+=(
    -v device_ip:$device_ip
    -v rte_ip:$rte_ip
    -v pikvm_ip:$pikvm_ip
    -v config:$config
    -v snipeit:no
)

if [ "$command" == "flash" ]; then
    "$script_dir/scp.py" "$rte_ip" "$fw_file" /tmp/dasharo.rom
    "$script_dir/ssh.py" "$rte_ip" /home/root/flash.sh /tmp/dasharo.rom
    exit 0
fi

case "$vendor" in
    msi)
        case "$model" in
            ms7d25_ddr5 | ms7e06_ddr5 | ms7e06_ddr5)
                compatibility_tests+=(
                    custom-boot-menu-key
                    esp-scanning
                    # Z690A DDR5 with current memmory modules do not support XMP profiles
                    # memory-profile
                    power-after-fail
                )

                security_tests+=(
                    me-neuter
                    option-rom
                    rebar
                )
                ;;
            *)
                echo unknown board: $model
                exit 1
                ;;
        esac
        ;;
    *)
        echo unknown vendor: $vendor
        exit 1
        ;;
esac

mkdir -p "test-results-$vendor-$model"

run_robot() {
    local category="$1"
    local test="$2"

    # We want to run all tests, even if some fail
    robot -l "test-results-$vendor-$model/dasharo-${category}.log.html" \
          -r "test-results-$vendor-$model/dasharo-${category}.html" \
          -o "test-results-$vendor-$model/dasharo-${category}.xml" \
          ${options[@]} "$root/dasharo-${category}/$test.robot" || true
}

if [ "$command" == "test" ]; then
    for test in "${compatibility_tests[@]}"; do
        run_robot compatibility "$test"
    done

    for test in "${performance_tests[@]}"; do
        run_robot performance "$test"
    done

    for test in "${security_tests[@]}"; do
        run_robot security "$test"
    done

    for test in "${stability_tests[@]}"; do
        run_robot stability "$test"
    done
fi
