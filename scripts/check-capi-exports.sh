#!/usr/bin/env bash
set -euo pipefail

LIB_PATH="${1:-}"

if [[ -z "$LIB_PATH" ]]; then
  echo "Usage: $0 /path/to/libtachyon.so"
  exit 2
fi

if [[ ! -f "$LIB_PATH" ]]; then
  echo "Library not found: $LIB_PATH"
  exit 2
fi

REQUIRED_SYMBOLS=(
  "tachyon_version"
  "tachyon_init"
  "tachyon_run"
  "tachyon_init_render_options"
  "tachyon_render"
)

if command -v nm >/dev/null 2>&1; then
  SYMBOLS="$(nm -D --defined-only "$LIB_PATH" || true)"
else
  echo "nm not found"
  exit 2
fi

for sym in "${REQUIRED_SYMBOLS[@]}"; do
  if ! echo "$SYMBOLS" | grep -q " $sym$"; then
    echo "Missing exported symbol: $sym"
    exit 1
  fi
done

echo "All required C API symbols are exported."
