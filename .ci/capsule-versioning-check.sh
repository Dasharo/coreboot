#!/bin/bash

# This script verifies that defconfig options related to capsule versions are
# properly updated between releases.  It expects tag of the new release/RC as
# its only parameter.

set -eu

# note that this works only for up to one release back in history
function get_previous_release() {
    local tag=$1

    local tag_prefix
    tag_prefix=$(echo "$tag" | grep -o '.*_v[0-9]')
    if [ "$tag" = "$tag_prefix" ]; then
        echo "error: failed to determine prefix of tag: '$tag'." 1>&2
        exit 1
    fi
    tag_prefix=${tag_prefix::-1}

    local pos
    if [[ $tag =~ .*-rc[0-9]+$ ]]; then
        # last non-RC release is the predecessor
        pos=1
    else
        # the previous non-RC release is the predecessor
        pos=2
    fi

    git tag --list "$tag_prefix*" --sort=-version:refname |
        grep -v '.*-rc[0-9]\+$' |
        sed -n "${pos}p"
}

function uses_v1_capsules() {
    local config=$1
    echo "$config" | grep -qFx "CONFIG_DRIVERS_EFI_UPDATE_CAPSULES=y" &&
        ! echo "$config" | grep -qFx "CONFIG_EDK2_CAPSULES_V2=y"
}

function uses_v2_capsules() {
    local config=$1
    echo "$config" | grep -qFx "CONFIG_EDK2_CAPSULES_V2=y"
}

function transitions_to_v2() {
    local config=$1
    echo "$config" | grep -qFx "CONFIG_EDK2_CAPSULES_V2_TRANSITION=y"
}

if [ $# -ne 1 ]; then
    echo "Usage: $(basename "$0") -h|--help|new-tag"
    exit 1
fi

if [ $1 = "-h" ] || [ $1 = "--help" ]; then
    echo "Usage: $(basename "$0") -h|--help|new-tag"
    exit 0
fi

new_tag=$1

old_tag=$(get_previous_release "$new_tag")
if [ -z "$old_tag" ]; then
    echo "warning: no previous release for '$new_tag', skipping checks." 1>&2
    exit 0
fi
echo "info: determined '$old_tag' to be the predecessor of '$new_tag'."

fail=0

# check all configs to not figure out mapping tag name to a config name, some
# false-positives are possible due to overlapping releases, but that should be
# rare and obvious to the user
mapfile -t configs < <(grep -l 'EFI_UPDATE_CAPSULE' configs/config.*)
for config in "${configs[@]}"; do
    new_config=$(git show "$new_tag:$config")
    if ! old_config=$(git show "$old_tag:$config"); then
        # `git show revision:path` fails if the revision doesn't contain the
        # path and we also have nothing to do in this case
        continue
    fi

    # check that `CONFIG_EDK2_CAPSULES_V2=y` is not added to
    # `CONFIG_DRIVERS_EFI_UPDATE_CAPSULES=y` without
    # `CONFIG_EDK2_CAPSULES_V2_TRANSITION=y`
    if uses_v1_capsules "$old_config" &&
       uses_v2_capsules "$new_config" &&
     ! transitions_to_v2 "$new_config"; then
        echo "error: '$config' switches to V2 without CONFIG_EDK2_CAPSULES_V2_TRANSITION=y" 1>&2
        fail=1
    fi

    # check that `CONFIG_EDK2_CAPSULES_V2_TRANSITION=y` doesn't live longer than
    # one release cycle
    if transitions_to_v2 "$old_config" &&
        transitions_to_v2 "$new_config"; then
        echo "error: '$config' didn't remove CONFIG_EDK2_CAPSULES_V2_TRANSITION=y after a release" 1>&2
        fail=1
    fi

    # check that `CONFIG_EDK2_CAPSULES_V2_TRANSITION=y` is not added to
    # `CONFIG_EDK2_CAPSULES_V2=y`
    if uses_v2_capsules "$old_config" &&
       uses_v2_capsules "$new_config" &&
       transitions_to_v2 "$new_config"; then
        echo "error: '$config' adds CONFIG_EDK2_CAPSULES_V2_TRANSITION=y to V2" 1>&2
        fail=1
    fi

    # check that `CONFIG_EDK2_CAPSULES_V2_TRANSITION=y` is not removed causing a
    # downgrade to V2
    if uses_v2_capsules "$old_config" &&
       uses_v1_capsules "$new_config"; then
        echo "error: '$config' removes CONFIG_EDK2_CAPSULES_V2=y" 1>&2
        fail=1
    fi
done

exit "$fail"
