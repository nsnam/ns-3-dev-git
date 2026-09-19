#!/usr/bin/env bash
# Provision the ndm-sys experiment container on M3 (wt-1-23).
#
# Usage (on M3):  REPO_DIR=~/ndm-sys bash container/provision.sh
# The repo (with this container/ dir) must already be at $REPO_DIR on M3.
#
# Result: image ndm-sys:latest + long-running container `ndm-sys` with /workspace bound to
# $WS_DIR (default ~/ndm-sys-workspace). Clone the git repo into $WS_DIR afterwards.
set -euo pipefail

REPO_DIR="${REPO_DIR:-$HOME/ndm-sys}"
IMAGE="ndm-sys:latest"
NAME="ndm-sys"
WS_DIR="${WS_DIR:-$HOME/ndm-sys-workspace}"

if [ ! -f "$REPO_DIR/container/Dockerfile" ]; then
    echo "ERROR: $REPO_DIR/container/Dockerfile not found (REPO_DIR=$REPO_DIR)" >&2
    exit 1
fi

echo "==> Building image $IMAGE"
docker build -t "$IMAGE" -f "$REPO_DIR/container/Dockerfile" "$REPO_DIR/container"

echo "==> Preparing workspace $WS_DIR"
mkdir -p "$WS_DIR"

echo "==> (Re)creating container $NAME"
docker rm -f "$NAME" 2>/dev/null || true
docker run -d --name "$NAME" --hostname ndm-sys --restart unless-stopped \
    -w /workspace -v "$WS_DIR:/workspace" "$IMAGE" sleep infinity

echo "==> Verifying toolchain inside the container"
docker exec "$NAME" zsh -lc 'echo container-ready; cmake --version | head -1; ninja --version; g++ --version | head -1'

echo
echo "Done. Next step (once the repo has a git remote or bundle):"
echo "  git clone <fork-url> $WS_DIR/ndm-sys      # or: git clone <bundle> $WS_DIR/ndm-sys"
echo "  docker exec -it ndm-sys zsh -c 'cd /workspace/ndm-sys && git log --oneline | head -3'"
