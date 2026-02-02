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
    python3.10-venv \
    rsync \
    symlinks \
    curl \
    unzip \
    wget \
    # Cross toolchain (CPU-side)
    gcc-aarch64-linux-gnu \
    g++-aarch64-linux-gnu

# Dev Tooling
RUN apt-get install -y --no-install-recommends \
    clangd \
    clang-format \
    git

# Leaving this here as I'm not 100% sure if I need them
# # CUDA Cross-Compilation Toolkits
# RUN wget https://developer.download.nvidia.com/compute/cuda/repos/ubuntu2204/cross-linux-sbsa/cuda-keyring_1.1-1_all.deb \
#     && dpkg -i cuda-keyring_1.1-1_all.deb \
#     && apt-get update \
#     && apt-get install -y --no-install-recommends \
#          cuda-cross-sbsa-12-6 \
#          cuda-toolkit-12-6 \
#     && rm -rf /var/lib/apt/lists/*

# Optional niceties
RUN useradd -m vscode && \
    mkdir -p /workspaces && chown -R vscode:vscode /workspaces

USER vscode
WORKDIR /workspaces
