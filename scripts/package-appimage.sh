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

# Pinned to release tags with checked sha256 digests. The "continuous" release
# assets are rebuilt in place, so an unpinned download broke this job without
# any change to this repository. To upgrade: pick a tag, copy its digest here.
LINUXDEPLOY_VERSION="1-alpha-20251107-1"
LINUXDEPLOY_SHA256="c20cd71e3a4e3b80c3483cef793cda3f4e990aca14014d23c544ca3ce1270b4d"
LINUXDEPLOY_URL="https://github.com/linuxdeploy/linuxdeploy/releases/download/${LINUXDEPLOY_VERSION}/linuxdeploy-x86_64.AppImage"
LINUXDEPLOY_QT_VERSION="1-alpha-20250213-1"
LINUXDEPLOY_QT_SHA256="15106be885c1c48a021198e7e1e9a48ce9d02a86dd0a1848f00bdbf3c1c92724"
LINUXDEPLOY_QT_URL="https://github.com/linuxdeploy/linuxdeploy-plugin-qt/releases/download/${LINUXDEPLOY_QT_VERSION}/linuxdeploy-plugin-qt-x86_64.AppImage"
LINUXDEPLOY="$TOOLS_DIR/linuxdeploy-x86_64.AppImage"
LINUXDEPLOY_QT="$TOOLS_DIR/linuxdeploy-plugin-qt-x86_64.AppImage"

die() { echo "error: $*" >&2; exit 1; }

[[ -x "$EXECUTABLE" ]] || die "missing $EXECUTABLE — build the project first (see README)"

QMAKE_BIN="$(command -v qmake6 || command -v qmake || true)"
[[ -n "$QMAKE_BIN" ]] || die "qmake6/qmake not found on PATH — needed by linuxdeploy-plugin-qt"

mkdir -p "$TOOLS_DIR" "$DIST"

# Downloads $url to $dest unless $dest already has the expected sha256, and
# refuses to install anything whose digest does not match.
fetch_tool() {
    local url="$1" dest="$2" sha256="$3"
    if [[ -x "$dest" ]] && echo "$sha256  $dest" | sha256sum -c --status -; then
        return
    fi
    echo "Downloading $(basename "$dest")..."
    local tmp="$dest.part"
    curl -fsSL "$url" -o "$tmp" || die "download failed: $url"
    if ! echo "$sha256  $tmp" | sha256sum -c --status -; then
        rm -f "$tmp"
        die "sha256 mismatch for $(basename "$dest") (expected $sha256)"
    fi
    mv "$tmp" "$dest"
    chmod +x "$dest"
}

fetch_tool "$LINUXDEPLOY_URL" "$LINUXDEPLOY" "$LINUXDEPLOY_SHA256"
fetch_tool "$LINUXDEPLOY_QT_URL" "$LINUXDEPLOY_QT" "$LINUXDEPLOY_QT_SHA256"

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
