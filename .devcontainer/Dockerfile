# Runs as x86_64 under Rosetta (compose forces linux/amd64)
FROM ubuntu:22.04

ARG DEBIAN_FRONTEND=noninteractive

# Base Tooling
RUN apt-get update && apt-get install -y --no-install-recommends \
    build-essential \
    cmake \
    ninja-build \
    pkg-config \
    file \
    ca-certificates \
    python3 \
    python3-pip \
    rsync \
    curl \
    unzip \
    # Cross toolchain (CPU-side)
    gcc-aarch64-linux-gnu \
    g++-aarch64-linux-gnu

# Dev Tooling
RUN apt-get install -y --no-install-recommends \
    clangd \
    clang-format \
    git \
    && rm -rf /var/lib/apt/lists/*

# Optional niceties
RUN useradd -m vscode && \
    mkdir -p /workspaces && chown -R vscode:vscode /workspaces
USER vscode
WORKDIR /workspaces
