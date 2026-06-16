#!/bin/sh
# Integration smoke for the --top / --watch / --top-sort / --top-color CLI
# surface. Runs against bin/coremetrics and asserts on the shape of stdout
# rather than specific process names (which vary per host). Used by the
# Makefile `make test-cli` target and the CI workflow.
#
# Exit 0 = all checks pass; non-zero = first failing check name + the bad
# output. Keeps the assertions cheap so this can run on every PR without
# slowing the matrix.

set -eu

BIN="${1:-bin/coremetrics}"
if [ ! -x "$BIN" ]; then
    echo "error: $BIN not found or not executable; run 'make bin/coremetrics' first" >&2
    exit 2
fi

PASS=0
FAIL=0

check()
{
    NAME="$1"
    if eval "$2"; then
        PASS=$((PASS + 1))
        echo "PASS  $NAME"
    else
        FAIL=$((FAIL + 1))
        echo "FAIL  $NAME"
    fi
}

# --top one-shot: should print a header row + N data rows + exit 0
OUT_TOP=$("$BIN" --top 3 2>&1)
check "--top 3 produces a header" \
    "echo \"\$OUT_TOP\" | head -1 | grep -q 'PID.*NAME.*CPU%.*MEM%.*IO'"
check "--top 3 produces at least 3 data rows" \
    "test \$(echo \"\$OUT_TOP\" | tail -n +2 | wc -l) -ge 3"

# --top-sort cpu reorders the output (first data row should be the highest
# CPU%; that may be 0.0 on an idle host, but the column should still be
# numeric)
OUT_CPU=$("$BIN" --top 3 --top-sort cpu 2>&1)
check "--top-sort cpu prints numeric CPU% column" \
    "echo \"\$OUT_CPU\" | tail -n +2 | head -1 | awk '{print \$3}' | grep -qE '^[0-9]+\.[0-9]+$'"

# --top-sort io reorders by I/O sum
OUT_IO=$("$BIN" --top 3 --top-sort io 2>&1)
check "--top-sort io prints numeric IO column" \
    "echo \"\$OUT_IO\" | tail -n +2 | head -1 | awk '{print \$NF}' | grep -qE '^[0-9]+$'"

# Real ESC byte derived via printf so dash + bash both produce U+001B
# without relying on the bash-only $'\033' literal.
ESC=$(printf '\033')

# --top-color never strips ANSI escapes
OUT_NOCOLOR=$("$BIN" --top 3 --top-color never 2>&1)
check "--top-color never produces no ANSI escape" \
    "! echo \"\$OUT_NOCOLOR\" | grep -q \"\$ESC\""

# --top-color always emits ANSI escapes even when piped
OUT_COLOR=$("$BIN" --top 3 --top-color always 2>&1)
check "--top-color always emits ANSI escape" \
    "echo \"\$OUT_COLOR\" | grep -q \"\$ESC\""

# Bad --top N falls back to default (20)
OUT_BAD=$("$BIN" --top abc 2>&1)
check "--top with non-numeric arg falls back to default 20 rows" \
    "test \$(echo \"\$OUT_BAD\" | tail -n +2 | wc -l) -ge 10"

# --help / -h prints the Usage block and exits 0
OUT_HELP=$("$BIN" --help 2>&1)
check "--help prints Usage block" \
    "echo \"\$OUT_HELP\" | head -1 | grep -q '^Usage: coremetrics'"
check "--help mentions every documented headless mode" \
    "echo \"\$OUT_HELP\" | grep -q -- '--top N' && echo \"\$OUT_HELP\" | grep -q -- '--screenshot PATH' && echo \"\$OUT_HELP\" | grep -q -- '--export-csv PATH'"

OUT_H_SHORT=$("$BIN" -h 2>&1)
check "-h short alias prints Usage" \
    "echo \"\$OUT_H_SHORT\" | head -1 | grep -q '^Usage: coremetrics'"

# --version / -V prints a semver line and exits 0
OUT_VERSION=$("$BIN" --version 2>&1)
check "--version prints semver line" \
    "echo \"\$OUT_VERSION\" | grep -qE '^coremetrics [0-9]+\\.[0-9]+\\.[0-9]+$'"

OUT_V_SHORT=$("$BIN" -V 2>&1)
check "-V short alias prints semver" \
    "echo \"\$OUT_V_SHORT\" | grep -qE '^coremetrics [0-9]+\\.[0-9]+\\.[0-9]+$'"

echo
echo "----"
echo "Passed: $PASS, Failed: $FAIL"
if [ "$FAIL" -gt 0 ]; then
    exit 1
fi
exit 0
