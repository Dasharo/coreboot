#!/usr/bin/env bash
# SPDX-FileCopyrightText: 2026 3mdeb <contact@3mdeb.com>
#
# SPDX-License-Identifier: Apache-2.0

set -euo pipefail

GITHUB_REPO="${GITHUB_REPO:?}"
GITHUB_TOKEN="${GITHUB_TOKEN:?}"
GITHUB_COMMIT_STATUS_TOKEN=${GITHUB_TOKEN:?} # Replace with dedicated one
BOARD="${BOARD:?}"
CI_COMMIT_SHA="${CI_COMMIT_SHA:?}"
CI_PIPELINE_URL="${CI_PIPELINE_URL:?}"

GH_API="https://api.github.com/repos/${GITHUB_REPO}"
GH_AUTH="Authorization: Bearer ${GITHUB_COMMIT_STATUS_TOKEN}"
CONTEXT="woodpecker/${BOARD}"

# https://docs.github.com/en/rest/commits/statuses#create-a-commit-status
publish_ci_status() { # $1=state, $2=context, $3=link_url, $4=description
    echo "Publishing status "$1" of "$2" to "$3""
    curl -sS --fail-with-body -X POST -H "$GH_AUTH" "${GH_API}/statuses/${CI_COMMIT_SHA}" \
        -d "$(jq -cn --arg state "$1" --arg context "$2" --arg target_url "$3" --arg description "$4" \
        '{state: $state, context: $context, target_url: $target_url, description: $description}')"
}

# Pushes an internal mirror commit with a given SHA so that CI result can be attached to it.
# Once the GH mirror is updated, the result will automatically attach to the
# commit
ensure_commit_on_github() {
    curl -sS --fail-with-body -H "$GH_AUTH" "${GH_API}/commits/${CI_COMMIT_SHA}" && return
    echo "Commit does not exist on GH yet, creating a pending status object"
    git push -q "https://x-access-token:${GITHUB_COMMIT_TOKEN}@github.com/${GITHUB_REPO}.git" \
        "${CI_COMMIT_SHA}:refs/ci-pending/${CI_COMMIT_SHA}"
}

cmd_init() {
    ensure_commit_on_github
    publish_ci_status pending "$CONTEXT" "$CI_PIPELINE_URL" "running"
}

cmd_publish() {
    WOODPECKER_TOKEN="${WOODPECKER_TOKEN:?}"
    CI_SYSTEM_URL="${CI_SYSTEM_URL:?}"
    CI_REPO_ID="${CI_REPO_ID:?}"
    CI_PIPELINE_NUMBER="${CI_PIPELINE_NUMBER:?}"

    local _pipeline _child_jobs _overall _job_name _job_state _job_pid

    _pipeline=$(curl -fsS -H "Authorization: Bearer ${WOODPECKER_TOKEN}" \
        "${CI_SYSTEM_URL}/api/repos/${CI_REPO_ID}/pipelines/${CI_PIPELINE_NUMBER}")

    jq_filter='
        .workflows[]
        | select(.environ.BOARD == $board)
        | .children[]
        | select(.name | test("^(report|cleanup)") | not)
        | [.name, .state, .pid]
        | @tsv
    '
    _child_jobs=$(jq -r --arg board "$BOARD" "$jq_filter" <<< "$_pipeline")

    _overall="success"
    local _state
    while IFS=$'\t' read -r _job_name _job_state _job_pid; do
        _state=""
        case "$_job_state" in
            failure | error | killed)
                _state="failure"
                _overall="failure"
                ;;
            success)
                _state="success"
                ;;
            *)
                continue
                ;;
        esac

        publish_ci_status "$_state" "${CONTEXT}/${_job_name}" "${CI_PIPELINE_URL}/${_job_pid}" "$_job_state"
    done <<< "$_child_jobs"

    publish_ci_status "$_overall" "${CONTEXT}" "${CI_PIPELINE_URL}" "$_overall"
}

case "${1:-}" in
    init) cmd_init ;;
    publish) cmd_publish ;;
    *)
        echo "usage: $0 init|publish" >&2
        exit 1
        ;;
esac
