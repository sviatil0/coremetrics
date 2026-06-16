#!/bin/sh
# release-notes.sh: print GitHub release notes for a tag from git log + conventional commits.
#
# Usage:
#   ./scripts/release-notes.sh              # default to latest tag (by version sort)
#   ./scripts/release-notes.sh v0.2.21      # explicit tag
#   ./scripts/release-notes.sh v0.2.21 | gh release create v0.2.21 --notes-file -
#
# Emits markdown to stdout. Groups commits between the previous tag (by version
# sort) and the target tag into Conventional Commit buckets and appends a
# Full Changelog compare link. POSIX sh (no bash-isms).

set -eu

REPO_URL="https://github.com/sviatil0/coremetrics"

# Bucket heading for a Conventional Commit type. Unknown types fall into Other.
bucket_for_type() {
    case "$1" in
        feat)       printf '%s\n' "Features" ;;
        fix)        printf '%s\n' "Bug Fixes" ;;
        docs)       printf '%s\n' "Documentation" ;;
        perf)       printf '%s\n' "Performance" ;;
        test)       printf '%s\n' "Tests" ;;
        bench)      printf '%s\n' "Benchmarks" ;;
        ci)         printf '%s\n' "CI" ;;
        packaging)  printf '%s\n' "Packaging" ;;
        refactor)   printf '%s\n' "Refactors" ;;
        chore)      printf '%s\n' "Chores" ;;
        release)    printf '%s\n' "Release" ;;
        *)          printf '%s\n' "Other" ;;
    esac
}

# Stable render order so the same input always produces the same output.
BUCKET_ORDER="Features Bug_Fixes Performance Documentation Tests Benchmarks CI Packaging Refactors Chores Release Other"

# Bucket names are stored with underscores in BUCKET_ORDER so the for-loop can
# split on whitespace. Convert back for display and filenames.
display_bucket() {
    # Replace underscores with spaces.
    printf '%s\n' "$1" | tr '_' ' '
}

# Extract the type prefix from a conventional commit subject.
# "feat(ui): foo" -> "feat"
# "docs: bar"     -> "docs"
# "no prefix"     -> "" (falls into Other)
type_of_subject() {
    head="${1%%:*}"
    if [ "$head" = "$1" ]; then
        printf '%s\n' ""
        return
    fi
    printf '%s\n' "${head%%(*}"
}

# Resolve the target tag: arg 1 if given, else the newest version-sorted tag.
TARGET="${1:-}"
if [ -z "$TARGET" ]; then
    TARGET=$(git tag --sort=-v:refname | head -n 1)
    if [ -z "$TARGET" ]; then
        printf 'release-notes: no tags in repo\n' >&2
        exit 1
    fi
fi

if ! git rev-parse --verify --quiet "refs/tags/$TARGET" >/dev/null; then
    printf 'release-notes: tag %s not found\n' "$TARGET" >&2
    exit 1
fi

# Previous tag = the tag immediately preceding TARGET in version sort order.
# `git tag --sort=v:refname` yields ascending semver order.
PREV=$(git tag --sort=v:refname | awk -v t="$TARGET" '
    { if ($0 == t) { print prev; exit } prev = $0 }
')

if [ -z "$PREV" ]; then
    # First-ever tag: walk from the root commit.
    RANGE=$(git rev-list --max-parents=0 HEAD | head -n 1)
    RANGE="$RANGE..$TARGET"
    COMPARE_NOTE=""
else
    RANGE="$PREV..$TARGET"
    COMPARE_NOTE="$PREV...$TARGET"
fi

# Commit subjects in the window, oldest first so bullets read chronologically.
# Drop merge commits so the notes stay focused on the actual changes.
subjects=$(git log --reverse --no-merges --pretty=format:'%s' "$RANGE" || true)

# Header.
printf "## What's new in %s\n\n" "$TARGET"

if [ -z "$subjects" ]; then
    printf 'No changes in this release.\n\n'
else
    # Per-bucket tmp files so we can preserve in-bucket order and emit buckets
    # in the canonical order.
    tmpdir=$(mktemp -d 2>/dev/null || mktemp -d -t relnotes)
    for b in $BUCKET_ORDER; do
        : > "$tmpdir/$b"
    done

    printf '%s\n' "$subjects" | while IFS= read -r subj; do
        [ -z "$subj" ] && continue
        t=$(type_of_subject "$subj")
        bucket=$(bucket_for_type "$t")
        # Bucket file uses the underscore form so it matches BUCKET_ORDER entries.
        key=$(printf '%s\n' "$bucket" | tr ' ' '_')
        printf '%s\n' "- $subj" >> "$tmpdir/$key"
    done

    for b in $BUCKET_ORDER; do
        f="$tmpdir/$b"
        if [ -s "$f" ]; then
            heading=$(display_bucket "$b")
            printf '### %s\n\n' "$heading"
            cat "$f"
            printf '\n'
        fi
    done

    rm -rf "$tmpdir"
fi

# Compare link. Use the previous tag when available; otherwise link to the tag.
if [ -n "$COMPARE_NOTE" ]; then
    printf '**Full Changelog**: %s/compare/%s\n' "$REPO_URL" "$COMPARE_NOTE"
else
    printf '**Full Changelog**: %s/commits/%s\n' "$REPO_URL" "$TARGET"
fi
