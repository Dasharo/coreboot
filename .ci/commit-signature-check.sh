#!/bin/bash
# SPDX-FileCopyrightText: 2026 3mdeb <contact@3mdeb.com>
#
# SPDX-License-Identifier: Apache-2.0
#
# Reject commits whose signature Gitea does not verify as a user's signature.
#
# For each commit the script asks Gitea for its signature verification
# (GET /repos/{owner}/{repo}/git/commits/{sha}). A commit passes only when
# Gitea reports it as verified and the key is not the Gitea instance key.
# Verified means the signature is valid and was made with a GPG or SSH key
# that a user uploaded to Gitea and that matches the committer's e-mail. The
# committer signs, so for rebased or cherry-picked patches this is not the
# author.
#
# This rejects unsigned commits, bad signatures, keys that nobody uploaded,
# and commits that Gitea created or re-signed with its own instance key
# (web editor, squash/rebase/merge-commit merges). Gitea reports those as
# verified too, with its configured signing name as the signer (e.g.
# "Gitea"), so they are recognised by the key ID in the verification reason
# ("<signer> / <key ID>"), which must not be one of GITEA_SIGNING_KEYS.
#
# Usage:
#   .ci/commit-signature-check.sh [<rev-list arguments>...]
#
# Without arguments the commit range is derived from the Woodpecker
# environment. Commits reachable from the upstream mirror branch are always
# excluded, because upstream coreboot commits are not signed.
#
# Requires: curl, jq, GITEA_TOKEN, CI_FORGE_URL and CI_REPO (owner/name).
# The commits must already be pushed to that repository.

set -euo pipefail

# Long key IDs of the Gitea instance signing key(s), separated by spaces.
GITEA_SIGNING_KEYS="${GITEA_SIGNING_KEYS:-3AB17144D0C6EAAC}"
UPSTREAM_BRANCH="${UPSTREAM_BRANCH:-main}"
REMOTE="${REMOTE:-origin}"
: "${GITEA_TOKEN:?GITEA_TOKEN must be set}"
: "${CI_FORGE_URL:?CI_FORGE_URL must be set}"
: "${CI_REPO:?CI_REPO must be set (owner/name)}"

for tool in curl jq; do
    if ! command -v "$tool" >/dev/null 2>&1; then
        echo "::error::$tool is not installed"
        exit 1
    fi
done

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

repo_api="${CI_FORGE_URL%/}/api/v1/repos/${CI_REPO}"
api="${repo_api}/git/commits"
response="$(mktemp)"
trap 'rm -f "$response"' EXIT

gitea_get() {
    curl -sS -o "$response" -w '%{http_code}' \
        -H "Authorization: token ${GITEA_TOKEN}" "$1" || true  # -w prints 000
}

api_message() {
    jq -r '.message // empty' "$response" 2>/dev/null | head -c 300 || true
}

# Fail early with a clear message if the API or the repository is not
# reachable with this token; Gitea answers 404 for private repositories the
# token cannot read.
status="$(gitea_get "$repo_api")"
if [[ "$status" != "200" ]]; then
    echo "::error::Cannot read ${CI_REPO} through the Gitea API"
    echo "::error::GET ${repo_api} returned HTTP ${status}: $(api_message)"
    echo "Check that CI_FORGE_URL (${CI_FORGE_URL}) is the Gitea URL and that"
    echo "GITEA_TOKEN can read ${CI_REPO} through the API (scope read:repository)."
    exit 1
fi

failed=0
for c in "${commits[@]}"; do
    subject="$(git log -1 --format='%h %s' "$c")"

    status="$(gitea_get "${api}/${c}?stat=false&files=false&verification=true")"
    if [[ "$status" != "200" ]]; then
        echo "::error::GET ${api}/${c} returned HTTP ${status}: $(api_message)"
        echo "::error::Could not check: $subject"
        failed=1
        continue
    fi

    # One field per line: whitespace-separated read would merge empty fields.
    mapfile -t v < <(jq -r '.commit.verification // {} |
        (.verified // false), (.reason // "" | gsub("\n"; " ")),
        (.signer.username // ""), (.signer.name // "" | gsub("\n"; " "))' \
        "$response")
    verified="${v[0]:-false}" reason="${v[1]:-}"
    signer_user="${v[2]:-}" signer_name="${v[3]:-}"
    # The reason of a verified signature is "<signer> / <key ID>".
    key="${reason##* / }"

    if [[ "$verified" != "true" ]]; then
        echo "::error::Signature not verified (${reason:-no reason given}): $subject"
        failed=1
    elif [[ -z "$signer_user" ||
            " ${GITEA_SIGNING_KEYS^^} " == *" ${key^^} "* ]]; then
        # Signatures made with the instance key verify too; Gitea reports
        # the configured signing name as the signer.
        echo "::error::Commit signed by the Gitea instance key (${signer_name:-unknown} / ${key}) instead of its committer: $subject"
        failed=1
    else
        echo "OK ${signer_user}: $subject"
    fi
done

if [[ $failed -ne 0 ]]; then
    cat <<EOF

Every commit must carry a signature that Gitea verifies as the committer's:
sign with a GPG or SSH key uploaded to your Gitea account, using the e-mail
address you commit with (git commit -S, git rebase -S, git cherry-pick -S).
Commits must never be signed by Gitea. Gitea signs commits made in the web
editor and commits it rewrites while merging a pull request. Create commits
locally, sign them and push them; merge pull requests with "Fast-forward only".
EOF
    exit 1
fi

echo "All ${#commits[@]} commit(s) carry a verified signature of a Gitea user."
