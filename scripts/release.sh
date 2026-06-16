#!/bin/sh
# release.sh: cut a new CoreMetrics release in one command.
#
# Usage:
#   ./scripts/release.sh patch       # 0.2.21 -> 0.2.22
#   ./scripts/release.sh minor       # 0.2.21 -> 0.3.0
#   ./scripts/release.sh major       # 0.2.21 -> 1.0.0
#   ./scripts/release.sh 0.2.42      # explicit override (no leading "v")
#
# What it does:
#   1. Parse current version from base.xml footer.
#   2. Compute new version from bump type (or accept explicit X.Y.Z).
#   3. Update the footer text in base.xml.
#   4. Stage + commit "release: vX.Y.Z footer version".
#   5. Push origin main.
#   6. Tag vX.Y.Z and push the tag.
#
# Preconditions:
#   - Current branch must be main.
#   - Working tree must be clean.
#
# POSIX sh (no bash-isms).

set -eu

GIT_AUTHOR_NAME="sviatil0"
GIT_AUTHOR_EMAIL="soleksiienko1@gmail.com"
BASE_XML="base.xml"

usage() {
    cat <<EOF
Usage: $0 <patch|minor|major|X.Y.Z>

Examples:
  $0 patch        # bump patch component
  $0 minor        # bump minor component, reset patch
  $0 major        # bump major component, reset minor and patch
  $0 0.2.42       # explicit version (no leading v)
EOF
}

if [ $# -ne 1 ]; then
    usage >&2
    exit 1
fi

ARG="$1"

# Must run from repo root (where base.xml lives).
if [ ! -f "$BASE_XML" ]; then
    echo "release.sh: $BASE_XML not found. Run from repo root." >&2
    exit 1
fi

# Must be on main.
CURRENT_BRANCH=$(git rev-parse --abbrev-ref HEAD)
if [ "$CURRENT_BRANCH" != "main" ]; then
    echo "release.sh: must be on main, currently on '$CURRENT_BRANCH'." >&2
    exit 1
fi

# Working tree must be clean.
if [ -n "$(git status --porcelain)" ]; then
    echo "release.sh: working tree is not clean. Commit or stash first." >&2
    git status --short >&2
    exit 1
fi

# Parse current version from base.xml footer.
CURRENT_RAW=$(grep -o "v0\\.[0-9]\\+\\.[0-9]\\+" "$BASE_XML" | head -1 || true)
if [ -z "$CURRENT_RAW" ]; then
    # Allow v1+ too; fall back to broader pattern.
    CURRENT_RAW=$(grep -o "v[0-9]\\+\\.[0-9]\\+\\.[0-9]\\+" "$BASE_XML" | head -1 || true)
fi
if [ -z "$CURRENT_RAW" ]; then
    echo "release.sh: could not parse current version from $BASE_XML." >&2
    exit 1
fi
CURRENT="${CURRENT_RAW#v}"

# Split current into major/minor/patch.
CUR_MAJOR=$(printf '%s' "$CURRENT" | cut -d. -f1)
CUR_MINOR=$(printf '%s' "$CURRENT" | cut -d. -f2)
CUR_PATCH=$(printf '%s' "$CURRENT" | cut -d. -f3)

# Compute new version.
case "$ARG" in
    patch)
        NEW_MAJOR="$CUR_MAJOR"
        NEW_MINOR="$CUR_MINOR"
        NEW_PATCH=$((CUR_PATCH + 1))
        ;;
    minor)
        NEW_MAJOR="$CUR_MAJOR"
        NEW_MINOR=$((CUR_MINOR + 1))
        NEW_PATCH=0
        ;;
    major)
        NEW_MAJOR=$((CUR_MAJOR + 1))
        NEW_MINOR=0
        NEW_PATCH=0
        ;;
    *)
        # Explicit X.Y.Z (no leading v). Validate strictly.
        if ! printf '%s' "$ARG" | grep -Eq '^[0-9]+\.[0-9]+\.[0-9]+$'; then
            echo "release.sh: '$ARG' is not patch|minor|major or a X.Y.Z version." >&2
            usage >&2
            exit 1
        fi
        NEW_MAJOR=$(printf '%s' "$ARG" | cut -d. -f1)
        NEW_MINOR=$(printf '%s' "$ARG" | cut -d. -f2)
        NEW_PATCH=$(printf '%s' "$ARG" | cut -d. -f3)
        ;;
esac

NEW="${NEW_MAJOR}.${NEW_MINOR}.${NEW_PATCH}"
NEW_TAG="v${NEW}"
CUR_TAG="v${CURRENT}"

if [ "$NEW_TAG" = "$CUR_TAG" ]; then
    echo "release.sh: new version $NEW_TAG matches current. Nothing to do." >&2
    exit 1
fi

# Refuse to overwrite an existing tag.
if git rev-parse "$NEW_TAG" >/dev/null 2>&1; then
    echo "release.sh: tag $NEW_TAG already exists locally." >&2
    exit 1
fi

echo "release.sh: bumping $CUR_TAG -> $NEW_TAG"

# Update the footer text in base.xml. Portable sed (BSD + GNU) via temp file.
TMP_XML="${BASE_XML}.release.tmp"
sed "s|<text>${CUR_TAG}</text>|<text>${NEW_TAG}</text>|" "$BASE_XML" > "$TMP_XML"

# Sanity check: the new tag must appear at least once in the rewritten file.
if ! grep -q "<text>${NEW_TAG}</text>" "$TMP_XML"; then
    rm -f "$TMP_XML"
    echo "release.sh: sed rewrite did not insert ${NEW_TAG} into ${BASE_XML}." >&2
    exit 1
fi
mv "$TMP_XML" "$BASE_XML"

# Stage and commit with explicit author identity.
git add "$BASE_XML"
git -c user.name="$GIT_AUTHOR_NAME" -c user.email="$GIT_AUTHOR_EMAIL" \
    commit -m "release: ${NEW_TAG} footer version"

# Push the release commit, then tag and push the tag.
git push origin main
git tag "$NEW_TAG"
git push origin "$NEW_TAG"

# Summary.
REMOTE_URL=$(git config --get remote.origin.url 2>/dev/null || echo "")
# Translate git@github.com:owner/repo.git -> https://github.com/owner/repo
case "$REMOTE_URL" in
    git@github.com:*)
        REPO_PATH=${REMOTE_URL#git@github.com:}
        REPO_PATH=${REPO_PATH%.git}
        RELEASE_URL="https://github.com/${REPO_PATH}/releases/new?tag=${NEW_TAG}"
        ;;
    https://github.com/*)
        REPO_PATH=${REMOTE_URL#https://github.com/}
        REPO_PATH=${REPO_PATH%.git}
        RELEASE_URL="https://github.com/${REPO_PATH}/releases/new?tag=${NEW_TAG}"
        ;;
    *)
        RELEASE_URL=""
        ;;
esac

echo
echo "release.sh: ${NEW_TAG} pushed."
if [ -n "$RELEASE_URL" ]; then
    echo "  draft release notes: $RELEASE_URL"
fi
echo "  brew bump:           brew bump-formula-pr --tag=${NEW_TAG} coremetrics"
