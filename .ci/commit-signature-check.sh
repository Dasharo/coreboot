#!/bin/bash
# SPDX-FileCopyrightText: 2026 3mdeb <contact@3mdeb.com>
#
# SPDX-License-Identifier: Apache-2.0
#
# Reject commits that are unsigned or signed with the Gitea instance key.
# Every commit must carry its author's own signature; commits that Gitea
# creates or re-signs (web editor, squash/rebase/merge-commit merges) are
# not allowed.
#
# Usage:
#   .ci/commit-signature-check.sh [<rev-list arguments>...]
#
# Without arguments the commit range is derived from the Woodpecker
# environment. Commits reachable from the upstream mirror branch are always
# excluded, because upstream coreboot commits are not signed.
#
# The public keys do not need to be in the keyring: the issuer key ID is read
# from the signature itself.

set -euo pipefail

# Long key IDs (or full fingerprints) of the Gitea signing key(s).
GITEA_SIGNING_KEYS="${GITEA_SIGNING_KEYS:-3AB17144D0C6EAAC}"
UPSTREAM_BRANCH="${UPSTREAM_BRANCH:-main}"
REMOTE="${REMOTE:-origin}"

if ! command -v gpg >/dev/null 2>&1; then
    echo "::error::gpg is not installed, cannot determine commit signing keys"
    exit 1
fi

is_valid_commit() {
    [[ -n "$1" && ! "$1" =~ ^0+$ ]] && git cat-file -e "$1^{commit}" 2>/dev/null
}

fetch_branch() {
    git fetch -q --no-tags "$REMOTE" \
        "+refs/heads/$1:refs/remotes/$REMOTE/$1"
}

if [[ $# -gt 0 ]]; then
    range=("$@")
else
    if [[ "${CI_PIPELINE_EVENT:-}" == "pull_request" ]]; then
        base="${CI_COMMIT_TARGET_BRANCH:?}"
    else
        base="${CI_REPO_DEFAULT_BRANCH:-dasharo}"
    fi

    fetch_branch "$base"
    excludes=()
    if fetch_branch "$UPSTREAM_BRANCH"; then
        excludes+=("$REMOTE/$UPSTREAM_BRANCH")
    else
        echo "::warning::Could not fetch '$UPSTREAM_BRANCH', upstream commits will not be excluded"
    fi

    if [[ "${CI_PIPELINE_EVENT:-}" != "pull_request" &&
          "${CI_COMMIT_BRANCH:-}" == "$base" ]]; then
        # Push to the base branch itself: check what was just pushed.
        prev="${CI_PREV_COMMIT_SHA:-}"
        if is_valid_commit "$prev"; then
            range=(HEAD --not "$prev" "${excludes[@]}")
        else
            range=(-1 HEAD)
        fi
    else
        range=(HEAD --not "$REMOTE/$base" "${excludes[@]}")
    fi
fi

echo "Checking commits: git rev-list ${range[*]}"

mapfile -t commits < <(git rev-list "${range[@]}")
if [[ ${#commits[@]} -eq 0 ]]; then
    echo "No commits to check."
    exit 0
fi

failed=0
for c in "${commits[@]}"; do
    subject="$(git log -1 --format='%h %s' "$c")"

    # Look at the raw object, so a missing gpg key or broken gpg setup can't
    # make a signed commit look unsigned or vice versa.
    if ! git cat-file commit "$c" | sed '/^$/q' | grep -qE '^gpgsig(-sha256)? '; then
        echo "::error::Unsigned commit: $subject"
        failed=1
        continue
    fi

    key="$(git log -1 --format='%GK' "$c" 2>/dev/null || true)"
    if [[ -z "$key" ]]; then
        echo "::error::Cannot determine signing key of: $subject"
        failed=1
        continue
    fi

    for gitea_key in $GITEA_SIGNING_KEYS; do
        k="${key^^}"; g="${gitea_key^^}"
        # Match a long key ID against a fingerprint in either direction.
        if [[ "$k" == *"$g" || "$g" == *"$k" ]]; then
            echo "::error::Commit signed by Gitea ($key) instead of its author: $subject"
            failed=1
            break
        fi
    done
done

if [[ $failed -ne 0 ]]; then
    cat <<EOF

Commits must be signed by their authors (git commit -S), never by Gitea.
Gitea signs commits made in the web editor and commits it rewrites while
merging a pull request. Create commits locally, sign them and push them;
merge pull requests with "Fast-forward only".
EOF
    exit 1
fi

echo "All ${#commits[@]} commit(s) are signed and none is signed by Gitea."
