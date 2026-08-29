#!/bin/bash
# =============================================================================
# RizkybyMONITOR — Linux Launcher Script
# =============================================================================
# Uses RELATIVE paths — works from any directory where the repo is cloned.
# Usage:
#   ./launcher.sh
#   bash launcher.sh
# =============================================================================

# Resolve the directory where this script lives (repo root), regardless of
# where it is called from.
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

# Display & backend environment
export PATH="/usr/bin:/usr/local/bin:$HOME/.local/bin:$PATH"
export DISPLAY="${DISPLAY:-:0}"
export WAYLAND_DISPLAY="${WAYLAND_DISPLAY:-wayland-0}"
export GDK_BACKEND=x11
export GTK_CSD=0

# Clean up any stale instances or port conflicts
fuser -k 8080/tcp > /dev/null 2>&1 || true
killall -9 rizkybymonitor_linux > /dev/null 2>&1 || true

# Move into the repo root so relative paths inside the binary work correctly
cd "$SCRIPT_DIR" || exit 1

# If the binary doesn't exist yet, try to build it first
if [ ! -f "$SCRIPT_DIR/rizkybymonitor_linux" ]; then
    echo "[launcher] Binary not found — building with make..."
    make -C "$SCRIPT_DIR" || { echo "[launcher] Build failed. Run 'make' manually."; exit 1; }
fi

# Launch the native C++17 binary
exec "$SCRIPT_DIR/rizkybymonitor_linux" "$@"
