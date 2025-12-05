#!/bin/sh
#
# GlyphOS ISO - Automated Build and Test Script
# Run this on FreeBSD 14.0+ system
#
# Usage: ./automated_build_test.sh [--skip-build] [--skip-test]

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

log_info() {
    echo "${GREEN}[INFO]${NC} $1"
}

log_warn() {
    echo "${YELLOW}[WARN]${NC} $1"
}

log_error() {
    echo "${RED}[ERROR]${NC} $1"
}

# Parse arguments
SKIP_BUILD=0
SKIP_TEST=0
for arg in "$@"; do
    case $arg in
        --skip-build) SKIP_BUILD=1 ;;
        --skip-test) SKIP_TEST=1 ;;
    esac
done

echo "==================================="
echo "GlyphOS ISO - Automated Build & Test"
echo "==================================="
echo

# Check if running on FreeBSD
if [ "$(uname -s)" != "FreeBSD" ]; then
    log_error "This script must run on FreeBSD 14.0+"
    log_info "Current OS: $(uname -s)"
    exit 1
fi

log_info "Running on $(uname -sr)"

# Check if root
if [ "$(id -u)" != "0" ]; then
    log_error "This script must be run as root"
    log_info "Try: sudo $0 $@"
    exit 1
fi

# Step 1: Install Prerequisites
log_info "Step 1: Installing prerequisites..."
if ! command -v pkg >/dev/null 2>&1; then
    log_warn "pkg not found, bootstrapping..."
    /usr/sbin/pkg bootstrap -y
fi

log_info "Installing required packages..."
pkg install -y cdrtools bash curl || pkg install -y xorriso bash curl

if ! command -v cargo >/dev/null 2>&1; then
    log_warn "Rust not installed. Installing via rustup..."
    if [ ! -f /usr/local/bin/rustup-init ]; then
        fetch -o /tmp/rustup-init.sh https://sh.rustup.rs
        sh /tmp/rustup-init.sh -y --default-toolchain stable
        . ~/.cargo/env || . /root/.cargo/env
    fi
fi

log_info "Prerequisites installed ✓"
echo

# Step 2: Build Rust Binaries
log_info "Step 2: Building Rust binaries..."
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
RUST_DIR="${SCRIPT_DIR}/../runtime/rust"

if [ ! -d "${RUST_DIR}" ]; then
    log_error "Rust runtime directory not found: ${RUST_DIR}"
    exit 1
fi

cd "${RUST_DIR}"
if [ ! -f "Cargo.toml" ]; then
    log_error "Cargo.toml not found in ${RUST_DIR}"
    exit 1
fi

log_info "Running cargo build --release --workspace..."
. ~/.cargo/env 2>/dev/null || . /root/.cargo/env 2>/dev/null || true
export PATH="$HOME/.cargo/bin:/root/.cargo/bin:$PATH"

cargo build --release --workspace 2>&1 | tail -20

if [ -f "target/release/glyphd" ] && [ -f "target/release/glyph-spu" ]; then
    log_info "Rust binaries built successfully ✓"
    ls -lh target/release/glyphd target/release/glyph-spu
else
    log_error "Failed to build Rust binaries"
    exit 1
fi

cd "${SCRIPT_DIR}"
echo

# Step 3: Build ISO
if [ $SKIP_BUILD -eq 0 ]; then
    log_info "Step 3: Building production ISO..."
    
    if [ ! -f "./build_iso.sh" ]; then
        log_error "build_iso.sh not found in ${SCRIPT_DIR}"
        exit 1
    fi
    
    chmod +x ./build_iso.sh
    
    log_info "Starting build (this may take 15-25 minutes)..."
    ./build_iso.sh 2>&1 | tee /tmp/glyphos_build.log
    
    if [ -f "glyphos-freebsd-0.1.0.iso" ]; then
        log_info "ISO built successfully ✓"
        ISO_SIZE=$(du -h glyphos-freebsd-0.1.0.iso | cut -f1)
        log_info "ISO size: ${ISO_SIZE}"
    else
        log_error "ISO file not found after build"
        log_info "Check /tmp/glyphos_build.log for details"
        exit 1
    fi
else
    log_warn "Skipping build (--skip-build specified)"
    if [ ! -f "glyphos-freebsd-0.1.0.iso" ]; then
        log_error "ISO file not found and build skipped"
        exit 1
    fi
fi

echo

# Step 4: Generate Checksum
log_info "Step 4: Generating checksum..."
sha256 glyphos-freebsd-0.1.0.iso > glyphos-freebsd-0.1.0.iso.sha256
CHECKSUM=$(cat glyphos-freebsd-0.1.0.iso.sha256)
log_info "SHA256: ${CHECKSUM}"
echo

# Step 5: Verify Artifacts
log_info "Step 5: Verifying artifacts..."
WORK_DIR="/tmp/glyphos-build"

if [ -d "${WORK_DIR}" ]; then
    log_info "Work directory structure:"
    find "${WORK_DIR}" -maxdepth 2 -type f -o -type d | head -20
else
    log_warn "Work directory not found (may have been cleaned)"
fi

if [ -f "glyphos-freebsd-0.1.0.iso" ]; then
    log_info "✓ glyphos-freebsd-0.1.0.iso"
else
    log_error "✗ glyphos-freebsd-0.1.0.iso MISSING"
fi

if [ -f "glyphos-freebsd-0.1.0.iso.sha256" ]; then
    log_info "✓ glyphos-freebsd-0.1.0.iso.sha256"
else
    log_error "✗ glyphos-freebsd-0.1.0.iso.sha256 MISSING"
fi

echo

# Step 6: Boot Test (if QEMU available and not skipped)
if [ $SKIP_TEST -eq 0 ]; then
    if command -v qemu-system-x86_64 >/dev/null 2>&1; then
        log_info "Step 6: Boot testing with QEMU..."
        log_warn "This will boot the ISO in QEMU (press Ctrl-C to exit)"
        log_info "Waiting 5 seconds before starting..."
        sleep 5
        
        qemu-system-x86_64 \
          -m 4G \
          -smp 2 \
          -cdrom glyphos-freebsd-0.1.0.iso \
          -boot d \
          -net nic \
          -net user,hostfwd=tcp::8080-:8080,hostfwd=tcp::8081-:8081,hostfwd=tcp::2222-:22,hostfwd=tcp::9100-:9100,hostfwd=tcp::9101-:9101 \
          -nographic
    else
        log_warn "QEMU not installed, skipping boot test"
        log_info "Install with: pkg install qemu"
    fi
else
    log_warn "Skipping boot test (--skip-test specified)"
fi

echo
echo "==================================="
log_info "Build and verification complete!"
echo "==================================="
echo
log_info "Next steps:"
echo "  1. Boot ISO in VM: qemu-system-x86_64 -cdrom glyphos-freebsd-0.1.0.iso -m 4G"
echo "  2. Verify services inside VM:"
echo "     service glyphd status"
echo "     service glyph_spu status"
echo "     curl http://localhost:8080/health"
echo "     curl http://localhost:8081/health"
echo "  3. Check security:"
echo "     pfctl -sr"
echo "     service sshd status"
echo "  4. Verify monitoring:"
echo "     curl http://localhost:9100/metrics"
echo "     curl http://localhost:9101/metrics"
echo

log_info "ISO ready: glyphos-freebsd-0.1.0.iso ($(du -h glyphos-freebsd-0.1.0.iso | cut -f1))"
log_info "Checksum: $(cat glyphos-freebsd-0.1.0.iso.sha256)"
