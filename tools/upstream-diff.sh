#!/bin/bash
# Compare a new upstream ATS-Mini release with the stored base (upstream-base/v2.45).
# Usage: tools/upstream-diff.sh <tag>   e.g. tools/upstream-diff.sh v2.46
set -e
TAG=${1:?usage: $0 <upstream tag>}
ROOT=$(cd "$(dirname "$0")/.." && pwd)
BASE="$ROOT/upstream-base/v2.45/ats-mini"
OURS="$ROOT/ats-mini"
TMP=$(mktemp -d); trap 'rm -rf "$TMP"' EXIT
git -C "$TMP" init -q
git -C "$TMP" fetch -q --depth 1 https://github.com/esp32-si4732/ats-mini.git "refs/tags/$TAG"
mkdir "$TMP/new" && git -C "$TMP" archive FETCH_HEAD ats-mini | tar -x -C "$TMP/new"
NEW="$TMP/new/ats-mini"
sel() { (cd "$1" && ls | grep -E '\.(cpp|h|ino)$' | sort); }
echo "== Upstream $TAG ($(git -C "$TMP" rev-parse --short FETCH_HEAD)) compared with the base =="
copy=(); merge=()
for f in $(comm -12 <(sel "$BASE") <(sel "$NEW")); do
  cmp -s "$BASE/$f" "$NEW/$f" && continue                      # unchanged upstream
  if cmp -s "$BASE/$f" "$OURS/$f" 2>/dev/null; then copy+=("$f"); else merge+=("$f"); fi
done
echo; echo "1) Changed upstream, untouched by the mod (copy as is):"; printf '   %s\n' "${copy[@]}"
echo; echo "2) Changed upstream AND by the mod (merge by hand):"; printf '   %s\n' "${merge[@]}"
echo; echo "3) Added upstream:";   comm -13 <(sel "$BASE") <(sel "$NEW") | sed 's/^/   /'
echo;  echo "   Removed upstream:"; comm -23 <(sel "$BASE") <(sel "$NEW") | sed 's/^/   /'
