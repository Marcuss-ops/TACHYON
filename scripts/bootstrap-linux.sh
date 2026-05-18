#!/usr/bin/env bash
set -euo pipefail

echo "[Tachyon] Bootstrapping compilation requirements..."
sudo apt-get update -q
sudo apt-get install -y --no-install-recommends \
  cmake \
  ninja-build \
  build-essential \
  git \
  pkg-config \
  python3 \
  libavcodec-dev \
  libavformat-dev \
  libavutil-dev \
  libswscale-dev \
  libswresample-dev \
  libfreetype6-dev \
  libharfbuzz-dev \
  libomp-dev \
  libsqlite3-dev \
  zlib1g-dev

echo "[Tachyon] Bootstrap completed successfully!"
