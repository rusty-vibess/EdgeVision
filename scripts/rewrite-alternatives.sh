#!/usr/bin/env bash
# Rewrite sysroot symlinks that point to /etc/alternatives/* so they point directly
# (and relatively) at the final target *within the sysroot*.
#
# Usage:
#   SYSROOT=/path/to/sysroot DRY_RUN=1 ./rewrite-alternatives.sh   # preview (default)
#   SYSROOT=/path/to/sysroot DRY_RUN=0 ./rewrite-alternatives.sh   # apply

set -euo pipefail

SYSROOT="${SYSROOT:-r36.4.4}"
DRY_RUN="${DRY_RUN:-1}"   # default to dry-run

# Non-dereferencing normalization (portable: macOS + Linux):
# absolute + normpath; does NOT follow symlinks.
py_normpath() {
  python3 - <<'PY' "$1"
import os, sys
print(os.path.normpath(os.path.abspath(sys.argv[1])))
PY
}

SYSROOT_NORM="$(py_normpath "$SYSROOT")"

# Must be absolute and must not contain '..' traversal segments.
safe_abs() { [[ "$1" == /* && "$1" != *"/../"* && "$1" != */.. ]]; }

# Boundary-safe "is under SYSROOT" check on normalized paths.
in_sysroot() {
  local p
  p="$(py_normpath "$1")"
  [[ "$p" == "$SYSROOT_NORM" || "$p" == "$SYSROOT_NORM/"* ]]
}

find \
  "$SYSROOT"/{lib,usr/lib,usr/include}* \
  -type l -lname '/etc/alternatives/*' -print0 2>/dev/null |
while IFS= read -r -d '' link; do
  in_sysroot "$link" || continue

  # link -> /etc/alternatives/...
  alt="$(readlink "$link" || true)"
  [[ -n "$alt" ]] || continue
  safe_abs "$alt" || continue

  alt_path="$SYSROOT$alt"
  [ -L "$alt_path" ] || continue
  in_sysroot "$alt_path" || continue

  # alt_path -> /usr/... or /lib/...
  final="$(readlink "$alt_path" || true)"
  [[ -n "$final" ]] || continue
  safe_abs "$final" || continue

  final_path="$SYSROOT$final"
  [ -e "$final_path" ] || continue
  in_sysroot "$final_path" || continue

  # Compute relative target from link dir -> final target (keeps sysroot relocatable)
  rel="$(python3 - <<'PY' "$(py_normpath "$link")" "$(py_normpath "$final_path")"
import os, sys
link_abs, final_abs = sys.argv[1], sys.argv[2]
print(os.path.relpath(final_abs, os.path.dirname(link_abs)))
PY
)"
  [[ -n "$rel" ]] || continue

  if [[ "$DRY_RUN" == "1" ]]; then
    printf 'DRY_RUN: %s -> %s\n' "$link" "$rel" >&2
  else
    ln -snf "$rel" "$link"
  fi
done
