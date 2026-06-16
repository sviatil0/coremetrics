#!/bin/sh
# lint-docs.sh: scan tracked *.md files for typography and whitespace issues.
#
# Usage:
#   ./scripts/lint-docs.sh           # advisory mode; only em/en-dashes fail
#   ./scripts/lint-docs.sh --strict  # any finding fails
#
# Hard violations (always exit 1):
#   * em-dash  (U+2014)
#   * en-dash  (U+2013)
#
# Advisory findings (exit 0 unless --strict):
#   * trailing whitespace on a line
#   * more than 2 consecutive blank lines
#   * mixed tabs and spaces inside fenced code blocks (visual indent only)
#
# Skipped directories: .git, node_modules, bin, obj.
# Output format: path:line: message
# POSIX sh, no bash-isms.

set -eu

STRICT=0
if [ "${1:-}" = "--strict" ]; then
    STRICT=1
fi

# Use a temp dir for per-file counters; the while-read loop runs in a subshell
# on POSIX sh, so plain variables cannot be propagated to the parent.
TMPDIR_LINT=$(mktemp -d 2>/dev/null || mktemp -d -t lint-docs)
trap 'rm -rf "$TMPDIR_LINT"' EXIT INT TERM
: > "$TMPDIR_LINT/hard"
: > "$TMPDIR_LINT/soft"

# Collect markdown files, skipping unwanted directories. Quoted -path patterns
# stay portable across BSD and GNU find.
md_files() {
    find . \
        \( -path './.git' -o -path './node_modules' -o -path './bin' -o -path './obj' \) -prune \
        -o -type f -name '*.md' -print
}

scan_file() {
    file=$1

    # --- Hard checks: em-dash and en-dash. ---
    # grep -n yields "LINE:content" which we reshape with awk. Set a counter
    # variable per match so we know whether to bump the hard tally.
    em_hits=$(grep -c -- '—' "$file" 2>/dev/null || true)
    em_hits=${em_hits:-0}
    if [ "$em_hits" -gt 0 ]; then
        grep -n -- '—' "$file" | awk -F: -v f="$file" '
            {
                line = $1
                $1 = ""
                sub(/^:/, "", $0)
                printf "%s:%s: em-dash found in line:%s\n", f, line, $0
            }
        '
        printf '%s\n' "$em_hits" >> "$TMPDIR_LINT/hard"
    fi

    en_hits=$(grep -c -- '–' "$file" 2>/dev/null || true)
    en_hits=${en_hits:-0}
    if [ "$en_hits" -gt 0 ]; then
        grep -n -- '–' "$file" | awk -F: -v f="$file" '
            {
                line = $1
                $1 = ""
                sub(/^:/, "", $0)
                printf "%s:%s: en-dash found in line:%s\n", f, line, $0
            }
        '
        printf '%s\n' "$en_hits" >> "$TMPDIR_LINT/hard"
    fi

    # --- Advisory checks: one awk pass per file. ---
    # Writes findings to stdout and the soft count to a per-file tmp file.
    awk -v f="$file" -v out="$TMPDIR_LINT/soft" '
        BEGIN {
            in_fence = 0
            blank_run = 0
            soft = 0
        }
        {
            line = $0

            # Trailing whitespace (space or tab) before EOL.
            if (line ~ /[ \t]+$/) {
                printf "%s:%d: trailing whitespace\n", f, NR
                soft++
            }

            # Consecutive blank lines: allow up to 2, flag the 3rd and beyond.
            if (line ~ /^[ \t]*$/) {
                blank_run++
                if (blank_run == 3) {
                    printf "%s:%d: more than 2 consecutive blank lines\n", f, NR
                    soft++
                }
            } else {
                blank_run = 0
            }

            # Track fenced code blocks delimited by ``` or ~~~ at line start
            # (optionally indented). Toggle on every fence line; never look
            # at fence lines themselves for indent.
            if (line ~ /^[ \t]*(```|~~~)/) {
                in_fence = 1 - in_fence
                next
            }

            # Inside a fence, flag indent that mixes tabs and spaces.
            if (in_fence) {
                indent = line
                sub(/[^ \t].*$/, "", indent)
                if (indent ~ /\t/ && indent ~ / /) {
                    printf "%s:%d: mixed tabs and spaces in code fence indent\n", f, NR
                    soft++
                }
            }
        }
        END {
            if (soft > 0) print soft >> out
        }
    ' "$file"
}

md_files | sort | while IFS= read -r f; do
    scan_file "$f"
done

# Sum the per-file counters.
HARD_HITS=$(awk '{ s += $1 } END { print s + 0 }' "$TMPDIR_LINT/hard")
SOFT_HITS=$(awk '{ s += $1 } END { print s + 0 }' "$TMPDIR_LINT/soft")

printf '\nlint-docs: %d hard violation(s), %d advisory finding(s)\n' \
    "$HARD_HITS" "$SOFT_HITS" >&2

if [ "$HARD_HITS" -gt 0 ]; then
    exit 1
fi

if [ "$STRICT" -eq 1 ] && [ "$SOFT_HITS" -gt 0 ]; then
    exit 1
fi

exit 0
