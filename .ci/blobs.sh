#!/usr/bin/env bash
# SPDX-FileCopyrightText: 2026 3mdeb <contact@3mdeb.com>
#
# SPDX-License-Identifier: Apache-2.0

REPO_ROOT=$(git rev-parse --show-toplevel)


BLOB_NAME="${BLOB_NAME:?}"
BLOB_METHOD="${BLOB_METHOD:?}"
BLOB_URL="${BLOB_URL:?}"

BLOB_PATH="${BLOB_PATH:-}"
BLOB_PATH="${REPO_ROOT}/${BLOB_PATH}"

ref_override="BLOB_REF_${BLOB_NAME}"
BLOB_REF="${!ref_override:-${BLOB_REF:-}}"

function die { # $1: msg
    printf "$1\n" >&2
    exit 1
}

function cmd_clone {
    [[ -n "$BLOB_REF" ]] || die "BLOB_REF required for BLOB_METHOD=clone"
    url="$BLOB_URL"
    if [ -n "$BLOB_TOKEN" ]; then
        token="${!BLOB_TOKEN}"
        [[ -n "$token" ]] || die "variable BLOB_TOKEN is empty or unset"
        url="${url/#https:\/\//https://ci:${token}@}"
    fi

    mkdir -p "$BLOB_PATH"
    git -C "$BLOB_PATH" init -q
    git -C "$BLOB_PATH" fetch -q --depth 1 "$url" "$BLOB_REF"
    git -C "$BLOB_PATH" checkout -q FETCH_HEAD
}

function cmd_get {
    curl -fsSL -o "$BLOB_PATH" "$BLOB_URL"
}

rm -rf "$BLOB_PATH"

case "$BLOB_METHOD" in
    clone)
        cmd_clone;
    ;;
    get)
        cmd_get;
    ;;
    *)
        die "Unknown method $BLOB_METHOD";
    ;;
esac

