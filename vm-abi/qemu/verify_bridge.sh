#!/bin/bash
# Copyright (C) 2026 AHMADALIPARR
# SPDX-License-Identifier: AGPL-3.0-only
# verify_bridge.sh — Reproducible build/verification for the HolyC QEMU bridge.
#
# Given a QEMU 8.2.2 tarball and a work directory, this script:
#   1. Extracts pristine QEMU 8.2.2 (sha256-verified).
#   2. Applies the HolyC bridge .patch files from patches/ (git apply, fail closed).
#   3. Copies the three bridge files (holyc_qemu_bridge.c/.h, holyc_qemu_traps.c)
#      into system/ (fail closed if any is missing).
#   4. Configures with the same options as the wired build.
#   5. Runs ninja -j2 qemu-system-x86_64, failing on any error or warning.
#   6. Confirms the bridge symbols are linked into the binary.
#   7. Prints a clear PASS/FAIL summary.
#
# Usage:
#   verify_bridge.sh [TARBALL] [WORKDIR]
#     TARBALL  Path to qemu-8.2.2.tar.xz (default: ~/workspace/.scratch/qemu/qemu-8.2.2.tar.xz)
#     WORKDIR  Scratch directory for the fresh tree (default: ~/workspace/.scratch/qemu/qemu-verify)
#
# The script never touches the monorepo and never builds in /tmp (RAM-backed).
# It needs only: tar, git (for git apply), ninja, meson, a C compiler.
# No Python is used by this script.
set -euo pipefail

TARBALL="${1:-$HOME/workspace/.scratch/qemu/qemu-8.2.2.tar.xz}"
WORKDIR="${2:-$HOME/workspace/.scratch/qemu/qemu-verify}"
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PATCH_DIR="$SCRIPT_DIR/patches"
EXPECTED_SHA256="847346c1b82c1a54b2c38f6edbd85549edeb17430b7d4d3da12620e2962bc4f3"
SRC="$WORKDIR/src"
BUILD="$SRC/build-verify"
LOG="$WORKDIR/verify_build.log"

fail() {
    echo "FAIL: $*" >&2
    exit 1
}

echo "== HolyC QEMU bridge verification =="
echo "Tarball: $TARBALL"
echo "Workdir: $WORKDIR"
echo "Patches: $PATCH_DIR"
echo "Bridge files: $SCRIPT_DIR"

# 1. Tarball exists + sha256 matches the verified QEMU 8.2.2 release.
[ -f "$TARBALL" ] || fail "tarball not found: $TARBALL"
ACTUAL_SHA256="$(sha256sum "$TARBALL" | awk '{print $1}')"
[ "$ACTUAL_SHA256" = "$EXPECTED_SHA256" ] || fail "tarball sha256 mismatch: got $ACTUAL_SHA256, expected $EXPECTED_SHA256"
echo "Tarball sha256 OK."

# 2. Fresh work dir (never /tmp — RAM-backed).
case "$WORKDIR" in
    /tmp|/tmp/*) fail "workdir must not be under /tmp (RAM-backed); use ~/workspace/.scratch/" ;;
esac
rm -rf "$WORKDIR"
mkdir -p "$SRC"

# 3. Extract pristine QEMU 8.2.2.
echo "Extracting pristine QEMU 8.2.2..."
tar --no-same-owner -xf "$TARBALL" -C "$SRC" --strip-components=1
[ -f "$SRC/configure" ] || fail "extract failed: no configure in $SRC"

# 4. Apply every .patch file from patches/ (sorted), fail closed on any.
echo "Applying patches..."
PATCH_COUNT=0
for p in "$PATCH_DIR"/*.patch; do
    [ -f "$p" ] || continue
    echo "  applying $(basename "$p")"
    (cd "$SRC" && git apply "$p") || fail "patch failed to apply: $p"
    PATCH_COUNT=$((PATCH_COUNT + 1))
done
[ "$PATCH_COUNT" -gt 0 ] || fail "no .patch files found in $PATCH_DIR"
echo "Applied $PATCH_COUNT patch(es)."

# 5. Copy the three bridge files into system/ (fail closed if any is missing).
echo "Copying bridge files..."
for f in holyc_qemu_bridge.c holyc_qemu_bridge.h holyc_qemu_traps.c; do
    [ -f "$SCRIPT_DIR/$f" ] || fail "bridge file missing: $SCRIPT_DIR/$f"
    cp "$SCRIPT_DIR/$f" "$SRC/system/$f"
done
grep -q 'mailbox_base = 0x90000' "$SRC/system/holyc_qemu_bridge.c" || fail "bridge file does not have mailbox_base 0x90000"
echo "Bridge files copied."

# 6. Configure with the wired build's exact options.
echo "Configuring..."
mkdir -p "$BUILD"
(cd "$BUILD" && ../configure \
    --target-list=x86_64-softmmu \
    --disable-werror \
    --disable-sdl --disable-gtk --disable-curses --disable-vnc \
    --disable-slirp --disable-nettle --disable-gnutls \
    --disable-virglrenderer --disable-spice --disable-brlapi \
    --disable-libnfs --disable-libusb --disable-usb-redir \
    --disable-vhost-user --disable-xen --disable-tools \
    --disable-guest-agent --disable-docs \
    --enable-plugins) > "$LOG" 2>&1 || { tail -40 "$LOG"; fail "configure failed (see $LOG)"; }
echo "Configure OK."

# 7. Build qemu-system-x86_64 with ninja -j2 (host RAM is limited).
echo "Building (ninja -j2 qemu-system-x86_64)..."
if ! (cd "$BUILD" && ninja -j2 qemu-system-x86_64) >> "$LOG" 2>&1; then
    tail -40 "$LOG"
    fail "ninja failed (see $LOG)"
fi
echo "ninja exit 0."

# 8. Fail closed on any warning or error in the build output.
WARN_COUNT="$(grep -c 'warning:' "$LOG" || true)"
ERR_COUNT="$(grep -c 'error:' "$LOG" || true)"
echo "Warnings: $WARN_COUNT, Errors: $ERR_COUNT"
[ "$WARN_COUNT" -eq 0 ] || { grep 'warning:' "$LOG" | head -20; fail "$WARN_COUNT warning(s) in build output"; }
[ "$ERR_COUNT" -eq 0 ] || { grep 'error:' "$LOG" | head -20; fail "$ERR_COUNT error(s) in build output"; }

# 9. Binary produced + bridge symbols linked in.
BIN="$BUILD/qemu-system-x86_64"
[ -x "$BIN" ] || fail "binary not produced: $BIN"
echo "Binary: $BIN ($(stat -c%s "$BIN") bytes)"
# Capture nm output to a file first. Piping nm into grep -q exits grep early,
# which SIGPIPE-kills nm; under 'set -o pipefail' that makes the pipeline
# fail even when the symbol IS present. Grepping a file avoids that.
SYMS_FILE="$WORKDIR/symbols.txt"
nm "$BIN" > "$SYMS_FILE" || fail "nm failed on $BIN"
for sym in global_holyc_env hmp_holyc_bridge holyc_bridge_activate holyc_DispatchIRQ holyc_install_traps; do
    grep -q " $sym$" "$SYMS_FILE" || fail "symbol missing from binary: $sym"
    echo "  symbol OK: $sym"
done
echo "Bridge symbols confirmed."

# 10. Summary.
echo "================ PASS ================"
echo "HolyC QEMU bridge verification PASSED."
echo "  Patches applied: $PATCH_COUNT"
echo "  Binary: $BIN"
echo "  Warnings: 0, Errors: 0"
echo "  Log: $LOG"
echo "======================================"
exit 0
