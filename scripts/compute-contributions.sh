#!/usr/bin/env bash
# Compute Stefan's line-count share across his source files using `git blame`.
#
# Walks src/ + include/ + bench/ + coremetrics.cpp, runs `git blame --line-porcelain`
# on each file, sums lines grouped by `author-mail`, and writes a shields.io
# `endpoint` JSON to .github/badges/contribution.json.
#
# The script is deterministic for a given commit (no clock-based randomness,
# no network calls), so two runs against the same tree produce byte-identical
# output. CI runs it on every push to main; the output JSON is committed back
# so the README badge can fetch it from raw.githubusercontent.

set -euo pipefail

ROOT="$(git rev-parse --show-toplevel)"
cd "$ROOT"

STEFAN_EMAIL="soleksiienko1@gmail.com"

# Source files included in the contribution count. Tests and assets are
# excluded so the percentage reflects the application code Stefan can be
# asked to defend in a Deep-Dive, not test scaffolding.
# Collect every source file via a NUL-delimited find pipeline. Portable to
# bash 3.2 (macOS default) so the script works for local runs without
# installing a newer shell. Tests and assets are excluded; the percentage
# reflects application code Stefan can be asked to defend, not scaffolding.
FILE_LIST="$(mktemp)"
trap 'rm -f "$FILE_LIST"' EXIT
find src include bench -type f \( -name '*.cpp' -o -name '*.hpp' -o -name '*.h' \) 2>/dev/null > "$FILE_LIST"
[ -f coremetrics.cpp ] && echo coremetrics.cpp >> "$FILE_LIST"

if [ ! -s "$FILE_LIST" ]; then
  echo "no source files found" >&2
  exit 1
fi

# The -w flag ignores whitespace-only churn so a reformat does not flip
# authorship for an entire file; -C and -M follow moves and copies so a
# function moved from one file to another keeps its original author.
TOTAL=0
STEFAN=0

while IFS= read -r f; do
  [ -n "$f" ] || continue
  if ! lines=$(git blame -w -C -M --line-porcelain -- "$f" 2>/dev/null); then
    continue
  fi
  file_total=$(printf '%s\n' "$lines" | grep -c '^author ' || true)
  file_stefan=$(printf '%s\n' "$lines" | grep -c "^author-mail <${STEFAN_EMAIL}>" || true)
  TOTAL=$((TOTAL + file_total))
  STEFAN=$((STEFAN + file_stefan))
done < "$FILE_LIST"

if [ "$TOTAL" -eq 0 ]; then
  echo "git blame returned 0 lines across the source tree" >&2
  exit 1
fi

PCT=$(awk -v s="$STEFAN" -v t="$TOTAL" 'BEGIN { printf("%.1f", (s / t) * 100) }')

# Color thresholds: red < 40, orange < 60, yellow < 75, green >= 75.
# Matches the existing bar threshold language in the UI so the README colors
# read consistently.
COLOR=$(awk -v p="$PCT" 'BEGIN {
  if (p < 40) print "red";
  else if (p < 60) print "orange";
  else if (p < 75) print "yellow";
  else print "brightgreen";
}')

OUT=".github/badges/contribution.json"
mkdir -p "$(dirname "$OUT")"

cat > "$OUT" <<JSON
{
  "schemaVersion": 1,
  "label": "Stefan's code",
  "message": "${PCT}% (${STEFAN} of ${TOTAL} lines)",
  "color": "${COLOR}"
}
JSON

echo "Wrote $OUT"
cat "$OUT"
