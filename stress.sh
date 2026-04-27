#!/bin/bash
# Stress test helper for CoreMetrics demo.
# Spawns CPU + RAM + GPU load so bars spike.
# Usage:
#   ./stress.sh              # default: 30s, 4 CPU workers, 512MB RAM, GPU if available
#   ./stress.sh 60 8 1024    # duration(sec), cpu workers, ram MB
#
# GPU stress requires one of:
#   brew install glmark2                (macOS)
#   apt install glmark2 stress-ng       (Linux)

DURATION="${1:-30}"
WORKERS="${2:-4}"
RAM_MB="${3:-512}"

echo "[stress] duration=${DURATION}s workers=${WORKERS} ram=${RAM_MB}MB"

PIDS=()

cleanup()
{
    echo "[stress] stopping..."
    for P in "${PIDS[@]}"; do
        kill "$P" 2>/dev/null
    done
    wait 2>/dev/null
    echo "[stress] done"
}
trap cleanup EXIT INT TERM

cpu_burn()
{
    local end=$(( $(date +%s) + DURATION ))
    while [ "$(date +%s)" -lt "$end" ]; do
        :
    done
}

ram_burn()
{
    # Allocate RAM_MB via Python if available, else yes into RAM via tmpfs-less fallback.
    if command -v python3 >/dev/null 2>&1; then
        python3 -c "
import time, os
mb = int(os.environ.get('RAM_MB', '512'))
dur = int(os.environ.get('DURATION', '30'))
buf = bytearray(mb * 1024 * 1024)
for i in range(0, len(buf), 4096):
    buf[i] = 1
time.sleep(dur)
"
    else
        # Fallback: shell array growth (slow, approximate)
        local tmp=()
        local end=$(( $(date +%s) + DURATION ))
        while [ "$(date +%s)" -lt "$end" ]; do
            tmp+=("$(head -c 1048576 /dev/urandom | base64)")
        done
    fi
}

gpu_burn()
{
    if command -v glmark2 >/dev/null 2>&1; then
        echo "[stress] GPU: glmark2"
        glmark2 --run-forever >/dev/null 2>&1 &
        PIDS+=("$!")
        return
    fi
    if command -v stress-ng >/dev/null 2>&1; then
        echo "[stress] GPU: stress-ng --gpu"
        stress-ng --gpu 1 --timeout "${DURATION}s" >/dev/null 2>&1 &
        PIDS+=("$!")
        return
    fi
    if [ "$(uname)" = "Darwin" ]; then
        echo "[stress] GPU: opening WebGL aquarium in browser"
        open "https://webglsamples.org/aquarium/aquarium.html"
        return
    fi
    echo "[stress] GPU: no stress tool found (install glmark2 or stress-ng, or open a WebGL page manually)"
}

for i in $(seq 1 "$WORKERS"); do
    cpu_burn &
    PIDS+=("$!")
done

DURATION="$DURATION" RAM_MB="$RAM_MB" ram_burn &
PIDS+=("$!")

gpu_burn

echo "[stress] running. Ctrl+C to abort."
wait
