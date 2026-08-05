#!/usr/bin/env bash
# Package the built factory-pulse Qt app as a self-contained AppImage.
# Requires: factory-pulse already built (see README's Build section).
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD="${FORGESIGHT_BUILD:-$ROOT/build}"
DIST="${FORGESIGHT_DIST:-$ROOT/dist}"
TOOLS_DIR="${FORGESIGHT_TOOLS_DIR:-$ROOT/.tools}"
APPDIR="$DIST/AppDir"
EXECUTABLE="$BUILD/ui/app/factory-pulse"

LINUXDEPLOY_URL="https://github.com/linuxdeploy/linuxdeploy/releases/download/continuous/linuxdeploy-x86_64.AppImage"
LINUXDEPLOY_QT_URL="https://github.com/linuxdeploy/linuxdeploy-plugin-qt/releases/download/continuous/linuxdeploy-plugin-qt-x86_64.AppImage"
LINUXDEPLOY="$TOOLS_DIR/linuxdeploy-x86_64.AppImage"
LINUXDEPLOY_QT="$TOOLS_DIR/linuxdeploy-plugin-qt-x86_64.AppImage"

die() { echo "error: $*" >&2; exit 1; }

[[ -x "$EXECUTABLE" ]] || die "missing $EXECUTABLE — build the project first (see README)"

QMAKE_BIN="$(command -v qmake6 || command -v qmake || true)"
[[ -n "$QMAKE_BIN" ]] || die "qmake6/qmake not found on PATH — needed by linuxdeploy-plugin-qt"

mkdir -p "$TOOLS_DIR" "$DIST"

fetch_tool() {
    local url="$1" dest="$2"
    if [[ -x "$dest" ]]; then
        return
    fi
    echo "Downloading $(basename "$dest")..."
    curl -sL "$url" -o "$dest"
    chmod +x "$dest"
}

fetch_tool "$LINUXDEPLOY_URL" "$LINUXDEPLOY"
fetch_tool "$LINUXDEPLOY_QT_URL" "$LINUXDEPLOY_QT"

# linuxdeploy shells out to `patchelf` to rewrite RPATHs; it's not always
# preinstalled and we don't assume sudo access, so fall back to the
# prebuilt-binary pip package (no apt/root needed) if it's missing.
if ! command -v patchelf >/dev/null 2>&1; then
    echo "patchelf not found, installing via pip --user..."
    python3 -m pip install --user --quiet patchelf
    export PATH="$HOME/.local/bin:$PATH"
    command -v patchelf >/dev/null 2>&1 || die "patchelf still not found after pip install"
fi

rm -rf "$APPDIR"
mkdir -p "$APPDIR"

# Lets linuxdeploy-plugin-qt's QML import scanner find exactly the Qt QML
# modules this app actually uses (QtQuick, QtQuick.Controls, QtCharts, ...)
# instead of guessing from linked libraries alone.
export QML_SOURCES_PATHS="$ROOT/ui/app/qml"
export QMAKE="$QMAKE_BIN"
# AppImages normally mount themselves via FUSE; fall back to extract+run so
# this also works in sandboxes/CI without /dev/fuse.
export APPIMAGE_EXTRACT_AND_RUN=1

cd "$DIST"
"$LINUXDEPLOY" \
    --appdir "$APPDIR" \
    --executable "$EXECUTABLE" \
    --desktop-file "$ROOT/packaging/forgesight.desktop" \
    --icon-file "$ROOT/packaging/icons/forgesight.svg" \
    --plugin qt \
    --output appimage

echo
echo "AppImage written to: $DIST"
ls -lh "$DIST"/*.AppImage
