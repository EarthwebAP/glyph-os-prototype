#!/bin/sh
#
# Build glyphos-freebsd-0.1.0.iso
#
# Requirements:
# - FreeBSD 14.0 or later
# - Root or sudo access
# - Built Rust binaries (glyphd, glyph-spu)

set -e

echo "=== GlyphOS FreeBSD ISO Builder ==="
echo

# Configuration
ISO_NAME="glyphos-freebsd-0.1.0.iso"
WORK_DIR="/tmp/glyphos-build"
OVERLAY_DIR="$(dirname $0)/overlay"

# Check if running as root
if [ "$(id -u)" != "0" ]; then
    echo "Error: This script must be run as root"
    exit 1
fi

# Clean previous build
rm -rf ${WORK_DIR}
mkdir -p ${WORK_DIR}

echo "Step 1: Creating base FreeBSD filesystem..."
# This would normally use freebsd-update or a pre-built base
# For now, create minimal structure
mkdir -p ${WORK_DIR}/{bin,sbin,usr,var,etc,tmp,dev}
mkdir -p ${WORK_DIR}/usr/local/{bin,etc/rc.d}
mkdir -p ${WORK_DIR}/var/{log,run}

echo "Step 2: Copying overlay files..."
if [ -d "${OVERLAY_DIR}" ]; then
    cp -R ${OVERLAY_DIR}/* ${WORK_DIR}/
else
    echo "Warning: Overlay directory not found: ${OVERLAY_DIR}"
fi

echo "Step 3: Installing Rust binaries..."
# Copy built binaries (assumes cargo build --release was run)
if [ -f "../runtime/rust/glyphd/target/release/glyphd" ]; then
    cp ../runtime/rust/glyphd/target/release/glyphd ${WORK_DIR}/usr/local/bin/
    chmod +x ${WORK_DIR}/usr/local/bin/glyphd
fi

if [ -f "../runtime/rust/glyph-spu/target/release/glyph-spu" ]; then
    cp ../runtime/rust/glyph-spu/target/release/glyph-spu ${WORK_DIR}/usr/local/bin/
    chmod +x ${WORK_DIR}/usr/local/bin/glyph-spu
fi

echo "Step 4: Configuring rc.conf..."
cat > ${WORK_DIR}/etc/rc.conf << 'EOFRC'
# GlyphOS Services
glyphd_enable="YES"
glyph_spu_enable="YES"

# Network
hostname="glyphos-node"
ifconfig_DEFAULT="DHCP"
sshd_enable="YES"
EOFRC

echo "Step 5: Creating glyphd user..."
# This would be done properly in the ISO post-install
mkdir -p ${WORK_DIR}/home/glyphd

echo "Step 6: Creating bootable ISO..."
# This is a placeholder - real ISO creation would use makefs and mkisofs
# mkisofs -R -b boot/cdboot -no-emul-boot -o ${ISO_NAME} ${WORK_DIR}

echo "Note: ISO creation requires additional FreeBSD build tools"
echo "For testing, use QEMU with the work directory directly:"
echo "  qemu-system-x86_64 -hda ${WORK_DIR} -m 2G -net nic -net user,hostfwd=tcp::8080-:8080,hostfwd=tcp::8081-:8081"

echo
echo "Build complete (staging)."
echo "Work directory: ${WORK_DIR}"
echo "Expected ISO: ${ISO_NAME}"
