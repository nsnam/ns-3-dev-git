#!/usr/bin/env bash
# ndm-sys project file — Apache License 2.0 (see LICENSE-PROJECT).
#
# tools/check.sh — the standing CI substitute (Phase 0.6).
#
# Runs (inside the ndm-sys container on M3, cwd = repo root):
#   1. configure + full build (CMake/Ninja, NS3_TESTS=ON)
#   2. all unit/system/performance test suites via `python3 test.py --no-build`
#      (the upstream CI mechanism; bare `ctest` runs every executable as a
#      ctest entry and fails on environment-bound ones like TAP/raw-socket
#      creators inside a container)
#   3. deterministic scenario pair: scratch/ndm-smoke-determinism twice with
#      the same seed must produce bit-identical metrics; a different seed must
#      not (guards against a seed that is not actually wired into the RNG).
#
# Exit 0 = green. A phase gate claim requires this to be green (PLAN.md).

set -euo pipefail
cd "$(git rev-parse --show-toplevel)"

JOBS="${JOBS:-$(nproc)}"
export CMAKE_BUILD_PARALLEL_LEVEL="$JOBS"

echo "== [1/3] configure + build (jobs=$JOBS)"
cmake -B build -G Ninja -DNS3_TESTS=ON
ninja -C build

echo "== [2/3] unit tests (test.py --no-build)"
mkdir -p testpy-output
python3 test.py --no-build -b build

echo "== [3/3] deterministic scenario pair"
SMOKE="$(find build/scratch -maxdepth 1 -name '*ndm-smoke-determinism*' -type f | head -1)"
if [ -z "$SMOKE" ]; then
    echo "FAIL: smoke app not built (expected build/scratch/*ndm-smoke-determinism*)"
    exit 1
fi

A1="$("$SMOKE" --seed=1)"
A2="$("$SMOKE" --seed=1)"
B1="$("$SMOKE" --seed=2)"
echo "  seed=1: $A1"
echo "  seed=2: $B1"

if [ "$A1" != "$A2" ]; then
    echo "FAIL: same seed produced different metrics (non-determinism)"
    exit 1
fi
if [ "$A1" = "$B1" ]; then
    echo "FAIL: different seeds produced identical metrics (seed not wired into RNG?)"
    exit 1
fi

echo "CHECK GREEN: build + unit tests + deterministic pair"
