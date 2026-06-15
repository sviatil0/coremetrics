#!/bin/sh
# gen-changelog.sh: regenerate CHANGELOG.md from git tags + conventional commits.
#
# Usage:
#   ./scripts/gen-changelog.sh                # print changelog to stdout
#   ./scripts/gen-changelog.sh > CHANGELOG.md # overwrite the tracked file
#
# Deterministic and idempotent: given the same git history it emits the same bytes.
# POSIX sh (no bash-isms). Group buckets follow Conventional Commits.

set -eu

# Tag selection: every annotated/lightweight tag matching v0.2.* in sort order.
# Starting tag (exclusive) for the v0.2.0 window is v0.1.1 so we capture the
# whole v0.2 series. Earlier tags are intentionally not enumerated.
START_TAG="v0.1.1"
TAG_GLOB="v0.2.*"

# Map a Conventional Commit type to its bucket heading.
# Anything unrecognized lands in "Other".
bucket_for_type() {
    case "$1" in
        feat)       printf '%s\n' "Features" ;;
        fix)        printf '%s\n' "Fixes" ;;
        docs)       printf '%s\n' "Documentation" ;;
        ci)         printf '%s\n' "CI" ;;
        chore)      printf '%s\n' "Chores" ;;
        bench)      printf '%s\n' "Benchmarks" ;;
        test)       printf '%s\n' "Tests" ;;
        packaging)  printf '%s\n' "Packaging" ;;
        perf)       printf '%s\n' "Performance" ;;
        refactor)   printf '%s\n' "Refactors" ;;
        release)    printf '%s\n' "Release" ;;
        *)          printf '%s\n' "Other" ;;
    esac
}

# Render order for buckets within a version section.
BUCKET_ORDER="Features Fixes Performance Documentation Tests Benchmarks CI Packaging Chores Refactors Release Other"

# Extract the type prefix from a conventional commit subject.
# "feat(ui): foo" -> "feat"
# "docs: bar"     -> "docs"
# "no prefix msg" -> "" (will fall into Other)
type_of_subject() {
    # POSIX: use parameter expansion for the first colon split, then strip scope.
    head="${1%%:*}"
    # If there was no colon, head == $1, treat as untyped.
    if [ "$head" = "$1" ]; then
        printf '%s\n' ""
        return
    fi
    # Strip "(scope)" suffix if present.
    printf '%s\n' "${head%%(*}"
}

# Emit the markdown for a single version window: prev..curr.
emit_version_section() {
    prev="$1"
    curr="$2"

    # Commit subjects in the window, newest first, no merge commits.
    subjects=$(git log "$prev".."$curr" --pretty=format:'%s' --no-merges || true)

    # Skip empty windows entirely (no section at all).
    if [ -z "$subjects" ]; then
        return
    fi

    # Section header with the tagged commit's short ISO date.
    date=$(git log -1 --format=%cs "$curr")
    printf '## %s - %s\n\n' "$curr" "$date"

    # Build a tmp file per bucket so we can preserve commit order within a bucket
    # and emit buckets in a fixed order. mktemp -d is POSIX-ish enough.
    tmpdir=$(mktemp -d 2>/dev/null || mktemp -d -t gencl)
    # Pre-create empty files for every known bucket so the order loop is simple.
    for b in $BUCKET_ORDER; do
        : > "$tmpdir/$b"
    done

    # Classify each subject into its bucket file.
    # Use a here-doc-fed while-read loop; works in POSIX sh.
    printf '%s\n' "$subjects" | while IFS= read -r subj; do
        [ -z "$subj" ] && continue
        t=$(type_of_subject "$subj")
        bucket=$(bucket_for_type "$t")
        # Append bullet. We do not rewrite the subject line itself; the
        # conventional prefix stays so the changelog reads like the log.
        printf '%s\n' "- $subj" >> "$tmpdir/$bucket"
    done

    # Emit non-empty buckets in the canonical order.
    for b in $BUCKET_ORDER; do
        f="$tmpdir/$b"
        if [ -s "$f" ]; then
            printf '### %s\n' "$b"
            cat "$f"
            printf '\n'
        fi
    done

    rm -rf "$tmpdir"
}

# Collect all v0.2.* tags in version sort order.
# `git tag --sort=v:refname` keeps numeric semver ordering.
tags=$(git tag --sort=v:refname --list "$TAG_GLOB")

if [ -z "$tags" ]; then
    printf 'gen-changelog: no tags matching %s\n' "$TAG_GLOB" >&2
    exit 1
fi

# Walk forward (oldest -> newest) to build a parallel "previous tag" list, then
# emit sections newest-first. We do this with two passes via tmp files so we
# avoid array syntax.
pairs_tmp=$(mktemp 2>/dev/null || mktemp -t gencl_pairs)
prev="$START_TAG"
for t in $tags; do
    printf '%s %s\n' "$prev" "$t" >> "$pairs_tmp"
    prev="$t"
done

# Reverse the pairs so newest version is first in the output.
# `tail -r` is BSD; `tac` is GNU. Try tac first, fall back to awk.
if command -v tac >/dev/null 2>&1; then
    reversed=$(tac "$pairs_tmp")
else
    reversed=$(awk '{ a[NR] = $0 } END { for (i = NR; i >= 1; i--) print a[i] }' "$pairs_tmp")
fi
rm -f "$pairs_tmp"

# Header.
printf '# Changelog\n\n'
printf 'Generated from git tags via `scripts/gen-changelog.sh`. Do not hand-edit;\n'
printf 'rerun the script to refresh.\n\n'

# Emit each section.
printf '%s\n' "$reversed" | while IFS=' ' read -r p c; do
    [ -z "$p" ] && continue
    emit_version_section "$p" "$c"
done
