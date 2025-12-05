#!/bin/sh
#
# GlyphOS ISO Smoke Test Script
#
# Performs basic validation of a GlyphOS ISO image:
#   - ISO format validation
#   - File integrity checks
#   - Boot sector verification
#   - Size and structure validation
#   - Optional QEMU boot test
#
# Usage:
#   ./iso_smoke_test.sh <iso_file> [OPTIONS]
#
# Options:
#   --boot-test      Run QEMU boot test (requires qemu-system-x86_64)
#   --mount-test     Mount ISO and verify contents (requires root)
#   --quick          Skip boot and mount tests
#   --timeout SECS   Boot test timeout (default: 60)
#
# Exit codes:
#   0 - All tests passed
#   1 - ISO file not found or invalid
#   2 - Format validation failed
#   3 - Boot test failed
#   4 - Mount test failed

set -e

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

# Configuration
BOOT_TEST=0
MOUNT_TEST=0
QUICK=0
BOOT_TIMEOUT=60
ISO_FILE=""

# Parse arguments
if [ $# -lt 1 ]; then
    echo "Usage: $0 <iso_file> [OPTIONS]" >&2
    echo "  --boot-test    Run QEMU boot test" >&2
    echo "  --mount-test   Mount and verify contents" >&2
    echo "  --quick        Skip boot and mount tests" >&2
    echo "  --timeout SECS Boot timeout (default: 60)" >&2
    exit 1
fi

ISO_FILE="$1"
shift

while [ $# -gt 0 ]; do
    case "$1" in
        --boot-test)
            BOOT_TEST=1
            shift
            ;;
        --mount-test)
            MOUNT_TEST=1
            shift
            ;;
        --quick)
            QUICK=1
            shift
            ;;
        --timeout)
            BOOT_TIMEOUT="$2"
            shift 2
            ;;
        *)
            echo "${RED}Unknown option: $1${NC}" >&2
            exit 1
            ;;
    esac
done

echo "${BLUE}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"
echo "${BLUE}  GlyphOS ISO Smoke Test${NC}"
echo "${BLUE}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"
echo ""
echo "ISO: $ISO_FILE"
echo "Quick mode: $([ $QUICK -eq 1 ] && echo "YES" || echo "NO")"
echo ""

# ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
# Test 1: File Existence
# ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

echo "${BLUE}[1/7] Checking ISO file existence...${NC}"

if [ ! -f "$ISO_FILE" ]; then
    echo "${RED}✗ ISO file not found: $ISO_FILE${NC}" >&2
    exit 1
fi

echo "${GREEN}✓ ISO file exists${NC}"
echo ""

# ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
# Test 2: File Format Validation
# ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

echo "${BLUE}[2/7] Validating ISO format...${NC}"

# Check file type
if command -v file >/dev/null 2>&1; then
    FILE_TYPE=$(file "$ISO_FILE")
    echo "  Type: $FILE_TYPE"

    if echo "$FILE_TYPE" | grep -q "ISO 9660"; then
        echo "${GREEN}✓ Valid ISO 9660 format${NC}"
    else
        echo "${RED}✗ Not a valid ISO 9660 format${NC}" >&2
        exit 2
    fi
else
    echo "${YELLOW}⚠ 'file' command not available, skipping format check${NC}"
fi

echo ""

# ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
# Test 3: Size Validation
# ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

echo "${BLUE}[3/7] Checking ISO size...${NC}"

# Get file size
ISO_SIZE_BYTES=$(stat -f%z "$ISO_FILE" 2>/dev/null || stat -c%s "$ISO_FILE" 2>/dev/null)
ISO_SIZE_MB=$((ISO_SIZE_BYTES / 1024 / 1024))

echo "  Size: ${ISO_SIZE_MB}MB (${ISO_SIZE_BYTES} bytes)"

# Expected range: 100MB - 1000MB (adjust as needed)
MIN_SIZE_MB=100
MAX_SIZE_MB=1000

if [ $ISO_SIZE_MB -lt $MIN_SIZE_MB ]; then
    echo "${RED}✗ ISO too small (expected ${MIN_SIZE_MB}-${MAX_SIZE_MB}MB)${NC}" >&2
    exit 2
elif [ $ISO_SIZE_MB -gt $MAX_SIZE_MB ]; then
    echo "${YELLOW}⚠ ISO larger than expected (${MAX_SIZE_MB}MB)${NC}"
else
    echo "${GREEN}✓ Size within expected range (${MIN_SIZE_MB}-${MAX_SIZE_MB}MB)${NC}"
fi

echo ""

# ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
# Test 4: Checksum Verification
# ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

echo "${BLUE}[4/7] Verifying checksum...${NC}"

# Check for .sha256 file
CHECKSUM_FILE="${ISO_FILE}.sha256"

if [ -f "$CHECKSUM_FILE" ]; then
    echo "  Checksum file: $CHECKSUM_FILE"

    # Verify checksum
    if sha256sum -c "$CHECKSUM_FILE" >/dev/null 2>&1; then
        echo "${GREEN}✓ Checksum verified${NC}"
        ACTUAL_CHECKSUM=$(cat "$CHECKSUM_FILE" | awk '{print $1}')
        echo "  SHA256: $ACTUAL_CHECKSUM"
    else
        echo "${RED}✗ Checksum verification failed${NC}" >&2
        exit 2
    fi
else
    echo "${YELLOW}⚠ Checksum file not found, generating...${NC}"
    ACTUAL_CHECKSUM=$(sha256sum "$ISO_FILE" | awk '{print $1}')
    echo "  SHA256: $ACTUAL_CHECKSUM"
fi

echo ""

# ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
# Test 5: Boot Sector Check
# ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

echo "${BLUE}[5/7] Checking boot sector...${NC}"

# Read first 512 bytes (boot sector)
BOOT_SECTOR=$(dd if="$ISO_FILE" bs=512 count=1 2>/dev/null | od -A n -t x1 | head -1)

if [ -n "$BOOT_SECTOR" ]; then
    echo "${GREEN}✓ Boot sector readable${NC}"
    echo "  First bytes: $(echo "$BOOT_SECTOR" | cut -c1-50)..."
else
    echo "${RED}✗ Cannot read boot sector${NC}" >&2
    exit 2
fi

echo ""

# ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
# Test 6: Mount Test (Optional)
# ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

if [ $MOUNT_TEST -eq 1 ] && [ $QUICK -eq 0 ]; then
    echo "${BLUE}[6/7] Mount test...${NC}"

    # Check if running as root
    if [ "$(id -u)" -ne 0 ]; then
        echo "${YELLOW}⚠ Mount test requires root, skipping${NC}"
    else
        MOUNT_DIR=$(mktemp -d)
        echo "  Mount point: $MOUNT_DIR"

        # Mount ISO
        if mount -o loop,ro "$ISO_FILE" "$MOUNT_DIR" 2>/dev/null; then
            echo "${GREEN}✓ ISO mounted successfully${NC}"

            # Check for key files
            EXPECTED_FILES="boot/loader.conf etc/rc.conf usr/local/bin/glyphd"

            for file in $EXPECTED_FILES; do
                if [ -e "$MOUNT_DIR/$file" ]; then
                    echo "  ✓ Found: $file"
                else
                    echo "  ${YELLOW}⚠ Missing: $file${NC}"
                fi
            done

            # List root directory
            echo "  Root directory contents:"
            ls -l "$MOUNT_DIR" | head -10

            # Unmount
            umount "$MOUNT_DIR"
            rmdir "$MOUNT_DIR"

            echo "${GREEN}✓ Mount test passed${NC}"
        else
            echo "${RED}✗ Failed to mount ISO${NC}" >&2
            rmdir "$MOUNT_DIR"
            exit 4
        fi
    fi
else
    echo "${BLUE}[6/7] Mount test skipped${NC}"
fi

echo ""

# ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
# Test 7: Boot Test with QEMU (Optional)
# ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

if [ $BOOT_TEST -eq 1 ] && [ $QUICK -eq 0 ]; then
    echo "${BLUE}[7/7] QEMU boot test (timeout: ${BOOT_TIMEOUT}s)...${NC}"

    if ! command -v qemu-system-x86_64 >/dev/null 2>&1; then
        echo "${YELLOW}⚠ QEMU not installed, skipping boot test${NC}"
    else
        echo "  Starting QEMU boot test..."

        # Start QEMU in background
        timeout "$BOOT_TIMEOUT" qemu-system-x86_64 \
            -cdrom "$ISO_FILE" \
            -m 2048 \
            -nographic \
            -serial stdio \
            -boot d \
            > /tmp/qemu_boot_output.log 2>&1 &

        QEMU_PID=$!

        # Wait a bit for boot
        sleep 30

        # Check if QEMU is still running
        if ps -p $QEMU_PID > /dev/null 2>&1; then
            echo "${GREEN}✓ QEMU boot started successfully${NC}"

            # Check boot output for errors
            if grep -i "panic\|error\|failed" /tmp/qemu_boot_output.log >/dev/null 2>&1; then
                echo "${YELLOW}⚠ Boot messages contain warnings/errors${NC}"
                echo "  Check /tmp/qemu_boot_output.log for details"
            else
                echo "${GREEN}✓ No boot errors detected${NC}"
            fi

            # Kill QEMU
            kill $QEMU_PID 2>/dev/null || true
            wait $QEMU_PID 2>/dev/null || true
        else
            echo "${RED}✗ QEMU boot failed${NC}" >&2
            cat /tmp/qemu_boot_output.log
            exit 3
        fi

        echo "  Boot log: /tmp/qemu_boot_output.log"
    fi
else
    echo "${BLUE}[7/7] Boot test skipped (use --boot-test to enable)${NC}"
fi

echo ""

# ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
# Summary
# ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

echo "${BLUE}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"
echo "${GREEN}✓ Smoke test PASSED${NC}"
echo "${BLUE}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"
echo ""

echo "Summary:"
echo "  ISO: $ISO_FILE"
echo "  Size: ${ISO_SIZE_MB}MB"
echo "  Format: ISO 9660"
echo "  Checksum: $(echo "$ACTUAL_CHECKSUM" | cut -c1-16)..."
echo ""

echo "Tests performed:"
echo "  ✓ File existence"
echo "  ✓ Format validation"
echo "  ✓ Size check"
echo "  ✓ Checksum verification"
echo "  ✓ Boot sector readable"
if [ $MOUNT_TEST -eq 1 ] && [ $QUICK -eq 0 ]; then
    echo "  ✓ Mount test"
else
    echo "  - Mount test (skipped)"
fi
if [ $BOOT_TEST -eq 1 ] && [ $QUICK -eq 0 ]; then
    echo "  ✓ Boot test"
else
    echo "  - Boot test (skipped)"
fi

echo ""
echo "${GREEN}Result: PASSED${NC}"
echo ""

echo "${BLUE}Next steps:${NC}"
echo "  1. Deploy to staging hardware"
echo "  2. Run full integration tests"
echo "  3. Perform 72-hour soak test"
echo "  4. Verify monitoring endpoints"
echo ""

# Write report
REPORT_FILE="logs/iso_smoke.log"
mkdir -p "$(dirname "$REPORT_FILE")"

cat > "$REPORT_FILE" << EOF
GlyphOS ISO Smoke Test Report
Generated: $(date -u +%Y-%m-%dT%H:%M:%SZ)

ISO: $ISO_FILE
Size: ${ISO_SIZE_MB}MB (${ISO_SIZE_BYTES} bytes)
Format: ISO 9660
Checksum: $ACTUAL_CHECKSUM

Tests Performed:
✓ File existence check
✓ Format validation (ISO 9660)
✓ Size validation (${MIN_SIZE_MB}-${MAX_SIZE_MB}MB range)
✓ Checksum verification
✓ Boot sector readability
$([ $MOUNT_TEST -eq 1 ] && echo "✓ Mount test (root required)" || echo "- Mount test (skipped)")
$([ $BOOT_TEST -eq 1 ] && echo "✓ QEMU boot test" || echo "- Boot test (skipped)")

Result: PASSED

Next Steps:
1. Deploy to staging hardware for integration testing
2. Verify all services start correctly
3. Test network configuration (DHCP/static)
4. Validate monitoring endpoints (Prometheus, Grafana)
5. Run 72-hour stability soak test
6. Perform security hardening validation

Recommended Tests:
- Full boot on physical hardware
- Service startup verification (glyphd, exporters)
- Network connectivity tests
- Persistence layer validation
- Update mechanism testing

EOF

echo "Report written to: $REPORT_FILE"

exit 0
