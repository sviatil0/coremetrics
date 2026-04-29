#!/bin/bash
# Run unit tests on Linux (via Docker) and macOS (native) to demonstrate cross-platform support.
# Usage: ./run-cross-platform-tests.sh

set -e

GREEN='\033[0;32m'
RED='\033[0;31m'
YELLOW='\033[1;33m'
NC='\033[0m'

PASS=0
FAIL=0

echo "=========================================="
echo "  Cross-Platform Test Runner"
echo "=========================================="

run_target()
{
    local name="$1"
    local cmd="$2"
    echo ""
    echo -e "${YELLOW}[$name]${NC} starting..."
    if eval "$cmd" > "/tmp/coremetrics_${name}.log" 2>&1; then
        if grep -q "All tests completed" "/tmp/coremetrics_${name}.log"; then
            echo -e "${GREEN}[$name] PASS${NC}"
            PASS=$((PASS + 1))
            return 0
        fi
    fi
    echo -e "${RED}[$name] FAIL${NC} (see /tmp/coremetrics_${name}.log)"
    FAIL=$((FAIL + 1))
    return 1
}

# macOS native
if [ "$(uname)" = "Darwin" ]; then
    run_target "macOS-native" "make clean && make test"
fi

# Linux via Docker
if command -v docker >/dev/null 2>&1; then
    if docker info >/dev/null 2>&1; then
        echo ""
        echo -e "${YELLOW}[Linux-docker]${NC} building image (cached on subsequent runs)..."
        docker build -f Dockerfile.linux -t coremetrics-linux . > /tmp/coremetrics_docker_build.log 2>&1
        run_target "Linux-docker" "docker run --rm -e SDL_VIDEO_DRIVER=dummy coremetrics-linux"
    else
        echo -e "${RED}[Linux-docker] Docker daemon not running, skipping${NC}"
        FAIL=$((FAIL + 1))
    fi
else
    echo -e "${RED}[Linux-docker] Docker not installed, skipping${NC}"
    FAIL=$((FAIL + 1))
fi

# Windows: Docker on Mac runs Linux containers only. Real Windows test requires Windows host or CI matrix.
echo ""
echo -e "${YELLOW}[Windows]${NC} skipped on macOS host (Windows containers require Windows host); covered by GitHub Actions CI matrix"

echo ""
echo "=========================================="
echo -e "Summary: ${GREEN}$PASS passed${NC}, ${RED}$FAIL failed${NC}"
echo "=========================================="
exit $FAIL
