# SPDX-FileCopyrightText: 2026 3mdeb <contact@3mdeb.com>
#
# SPDX-License-Identifier: Apache-2.0

FROM ghcr.io/dasharo/dasharo-sdk:v1.9.5

USER root

ARG DOCKER_VERSION=28.3.3
RUN curl -fsSL "https://download.docker.com/linux/static/stable/x86_64/docker-${DOCKER_VERSION}.tgz" \
        | tar -xz -C /usr/local/bin --strip-components=1 docker/docker

ARG TASK_VERSION=3.54.0
RUN curl -fsSL "https://github.com/go-task/task/releases/download/v${TASK_VERSION}/task_linux_amd64.tar.gz" \
      | tar -xz -C /usr/local/bin task

RUN git config --system --add safe.directory '*'

WORKDIR /workspace
