#!/usr/bin/env bash
# Build amplitude and install the binary. Defaults to a user-local install
# (~/.local/bin, no root). Pass --system to install to /usr/local/bin.
set -euo pipefail

PREFIX="$HOME/.local"
SYSTEM=0
for arg in "$@"; do
    case "$arg" in
        --system) SYSTEM=1; PREFIX="/usr/local" ;;
        -h|--help)
            echo "usage: ./install.sh [--system]"
            echo "  (default) install to ~/.local/bin"
            echo "  --system  install to /usr/local/bin (uses sudo)"
            exit 0 ;;
        *) echo "unknown option: $arg" >&2; exit 1 ;;
    esac
done

for dep in meson ninja g++; do
    command -v "$dep" >/dev/null || { echo "missing build dependency: $dep" >&2; exit 1; }
done

cd "$(dirname "$0")"
echo ":: building"
meson setup build --reconfigure >/dev/null
meson compile -C build >/dev/null

BIN="$PREFIX/bin/amplitude"
echo ":: installing -> $BIN"
if [ "$SYSTEM" -eq 1 ]; then
    sudo install -Dm755 build/amplitude "$BIN"
else
    install -Dm755 build/amplitude "$BIN"
fi

echo ":: done"
echo
echo "Run it:        amplitude --out /tmp/amplitude.json --interval 1000"
echo "Autostart (Hyprland):"
echo "  exec-once = amplitude --out /tmp/amplitude.json --interval 1000"
if [ "$SYSTEM" -eq 0 ]; then
    case ":$PATH:" in
        *":$HOME/.local/bin:"*) ;;
        *) echo
           echo "note: ~/.local/bin is not on your PATH. Add it, e.g.:"
           echo "  fish_add_path ~/.local/bin     # fish"
           echo "  export PATH=\"\$HOME/.local/bin:\$PATH\"   # bash/zsh" ;;
    esac
fi
