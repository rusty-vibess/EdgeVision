#!/usr/bin/env bash
set -euo pipefail

BUILD_DIR="${1:?build dir required}"
PATCH_DIR="${2:?patch dir required}"

shopt -s nullglob
patches=("$PATCH_DIR"/*.patch)
((${#patches[@]})) || { echo "No patches in $PATCH_DIR" >&2; exit 0; }

for p in "${patches[@]}"; do
  echo "Applying $(basename "$p")"
  patch --verbose -p0 -N -i "$p" -d "$BUILD_DIR" || true
done
