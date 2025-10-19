#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BIN_DIR="${ROOT_DIR}/build/fast-debug/bin"
EXE_NAME="snake_game"
EXE_PATH="${BIN_DIR}/${EXE_NAME}"

if [[ ! -x "$EXE_PATH" ]]; then
    echo "❌ Không tìm thấy file thực thi ở $EXE_PATH"
    echo "💡 Bạn đã chạy './scripts/build_fast.sh' chưa?"
    exit 1
fi

# Rất quan trọng: Chạy game từ bên trong thư mục 'bin'
# để nó có thể tìm thấy assets (CascadiaCode.ttf, assets/style.css)
cd "$BIN_DIR"

echo "==> Đang chạy ${EXE_NAME}..."
echo "========================================="
./"$EXE_NAME"