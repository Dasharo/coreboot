# SPDX-FileCopyrightText: 2026 3mdeb <contact@3mdeb.com>
#
# SPDX-License-Identifier: Apache-2.0

FROM docker:cli

RUN apk add --no-cache bash git curl

ARG TASK_VERSION=3.54.0
RUN curl -fsSL "https://github.com/go-task/task/releases/download/v${TASK_VERSION}/task_linux_amd64.tar.gz" \
      | tar -xz -C /usr/local/bin task

RUN git config --system --add safe.directory '*'

WORKDIR /workspace
