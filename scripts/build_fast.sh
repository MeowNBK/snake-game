#!/usr/bin/env bash
set -euo pipefail

# fast build driver: configure + build using preset fast-debug
ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT_DIR"

# Ensure ccache environment mildly tuned
export CCACHE_CPP2=yes

# configure (will reuse existing config if unchanged)
echo "==> cmake configure --preset fast-debug"
cmake --preset fast-debug

# build snake_game (thay vì meow-vm) with maximum parallelism
NPROCS="$(nproc || echo 2)"
echo "==> build (ninja -j${NPROCS})"
cmake --build --preset fast-debug -- -j"${NPROCS}" snake_game

echo
echo "Build finished. Executable at build/fast-debug/bin/snake_game"