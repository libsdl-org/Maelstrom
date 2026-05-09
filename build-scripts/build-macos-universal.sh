#!/bin/bash
# build-macos-universal.sh — Build a universal (arm64 + x86_64) Maelstrom.app for macOS
#
# Tested on: Apple Mac Mini M4 (macOS Sequoia)
# Built with the assistance of Claude Code (https://claude.ai/claude-code)
#
# Usage:
#   ./build-scripts/build-macos-universal.sh [--output /path/to/output]
#
# Requirements:
#   - Xcode Command Line Tools (xcode-select --install)
#   - CMake 3.0+ (brew install cmake)
#
# Note: Steam support is excluded from this build. The SteamworksSDK submodule
# requires separate access. Steam features are available via the official Steam release.
#
# Note: On first launch, macOS will prompt for permission to access your Documents
# folder. This is expected — SDL3's user storage API saves preferences and high
# scores there. Grant access to enable saving.

set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
REPO_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"
OUTPUT_DIR="${1:-$REPO_DIR}"

# Parse --output argument
while [[ $# -gt 0 ]]; do
    case "$1" in
        --output) OUTPUT_DIR="$2"; shift 2 ;;
        *) shift ;;
    esac
done

echo "==> Building Maelstrom universal macOS app"
echo "    Repo:   $REPO_DIR"
echo "    Output: $OUTPUT_DIR"
echo ""

# --- Prerequisites ---
if ! command -v cmake &>/dev/null; then
    echo "Error: cmake not found. Install with: brew install cmake"
    exit 1
fi

if ! xcode-select -p &>/dev/null; then
    echo "Error: Xcode Command Line Tools not found. Install with: xcode-select --install"
    exit 1
fi

cd "$REPO_DIR"

# --- Submodules ---
echo "==> Initializing submodules (skipping SteamworksSDK)..."
git submodule update --init external/SDL external/SDL_net external/physfs

# SDL_net may need manual checkout if the working tree is empty
if [ ! -f external/SDL_net/CMakeLists.txt ]; then
    echo "    Manually checking out SDL_net..."
    SDLNET_COMMIT=$(git submodule status external/SDL_net | awk '{print $1}' | tr -d '-')
    GIT_WORK_TREE=external/SDL_net GIT_DIR=.git/modules/external/SDL_net \
        git checkout "$SDLNET_COMMIT" -- .
fi

# --- Build arm64 ---
echo ""
echo "==> Configuring for arm64..."
cmake -B build-arm64 \
    -DCMAKE_OSX_ARCHITECTURES=arm64 \
    -DCMAKE_BUILD_TYPE=Release \
    -DSTEAM=OFF

echo "==> Building for arm64..."
cmake --build build-arm64 --config Release

# --- Build x86_64 ---
echo ""
echo "==> Configuring for x86_64..."
cmake -B build-x86_64 \
    -DCMAKE_OSX_ARCHITECTURES=x86_64 \
    -DCMAKE_BUILD_TYPE=Release \
    -DSTEAM=OFF

echo "==> Building for x86_64..."
cmake --build build-x86_64 --config Release

# --- Stage game data ---
echo ""
echo "==> Staging game data..."
cmake --install build-arm64 --prefix stage

# --- Create universal binaries ---
echo ""
echo "==> Creating universal binaries with lipo..."
lipo -create \
    build-arm64/Release/Maelstrom \
    build-x86_64/Release/Maelstrom \
    -output stage/Maelstrom

lipo -create \
    build-arm64/Release/libSDL3.0.dylib \
    build-x86_64/Release/libSDL3.0.dylib \
    -output stage/libSDL3.0.dylib

# --- Assemble .app bundle ---
echo ""
echo "==> Assembling Maelstrom.app..."

APP="$OUTPUT_DIR/Maelstrom.app"
rm -rf "$APP"
mkdir -p "$APP/Contents/MacOS"
mkdir -p "$APP/Contents/Resources"

# Binary and dylib go in MacOS/
cp stage/Maelstrom "$APP/Contents/MacOS/"
cp stage/libSDL3.0.dylib "$APP/Contents/MacOS/"

# Game data goes in Resources/
cp -r stage/Data "$APP/Contents/Resources/"
cp -r stage/mods "$APP/Contents/Resources/"

# Icon (sourced from Xcode assets)
ICNS="$REPO_DIR/Xcode/Assets.xcassets/AppIcon.appiconset"
if [ -f "$ICNS/maelstrom1024.png" ]; then
    # Convert PNG to icns if iconutil is available
    ICONSET=$(mktemp -d).iconset
    mkdir -p "$ICONSET"
    sips -z 16 16 "$ICNS/maelstrom1024.png" --out "$ICONSET/icon_16x16.png" &>/dev/null
    sips -z 32 32 "$ICNS/maelstrom1024.png" --out "$ICONSET/icon_16x16@2x.png" &>/dev/null
    sips -z 32 32 "$ICNS/maelstrom1024.png" --out "$ICONSET/icon_32x32.png" &>/dev/null
    sips -z 64 64 "$ICNS/maelstrom1024.png" --out "$ICONSET/icon_32x32@2x.png" &>/dev/null
    sips -z 128 128 "$ICNS/maelstrom1024.png" --out "$ICONSET/icon_128x128.png" &>/dev/null
    sips -z 256 256 "$ICNS/maelstrom1024.png" --out "$ICONSET/icon_128x128@2x.png" &>/dev/null
    sips -z 256 256 "$ICNS/maelstrom1024.png" --out "$ICONSET/icon_256x256.png" &>/dev/null
    sips -z 512 512 "$ICNS/maelstrom1024.png" --out "$ICONSET/icon_256x256@2x.png" &>/dev/null
    sips -z 512 512 "$ICNS/maelstrom1024.png" --out "$ICONSET/icon_512x512.png" &>/dev/null
    cp "$ICNS/maelstrom1024.png" "$ICONSET/icon_512x512@2x.png"
    iconutil -c icns "$ICONSET" -o "$APP/Contents/Resources/Maelstrom.icns"
    rm -rf "$ICONSET"
fi

# Info.plist — substitute version from CMakeLists.txt
MAJOR=$(grep "^set(MAJOR_VERSION" "$REPO_DIR/CMakeLists.txt" | grep -o '[0-9]*')
MINOR=$(grep "^set(MINOR_VERSION" "$REPO_DIR/CMakeLists.txt" | grep -o '[0-9]*')
MICRO=$(grep "^set(MICRO_VERSION" "$REPO_DIR/CMakeLists.txt" | grep -o '[0-9]*')
VERSION="$MAJOR.$MINOR.$MICRO"

sed "s/MAELSTROM_VERSION/$VERSION/g" \
    "$REPO_DIR/build-scripts/macos/Info.plist" \
    > "$APP/Contents/Info.plist"

printf 'APPL????' > "$APP/Contents/PkgInfo"

# --- Done ---
echo ""
echo "==> Done!"
echo ""
echo "    $APP"
echo ""
file "$APP/Contents/MacOS/Maelstrom"
echo ""
echo "To install, run:"
echo "    cp -r \"$APP\" /Applications/"
