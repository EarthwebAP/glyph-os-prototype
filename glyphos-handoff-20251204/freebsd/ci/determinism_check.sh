#!/bin/sh
#
# GlyphOS Determinism Parity Check
# Version: 1.0.0
#
# Verifies that repeated builds produce bit-identical binaries
#
# Usage: ./ci/determinism_check.sh
#
# Exit codes:
#   0 - Builds are deterministic
#   1 - Builds are non-deterministic
#   2 - Build failed
#

set -e

# Colors
if [ -t 1 ]; then
    RED='\033[0;31m'
    GREEN='\033[0;32m'
    YELLOW='\033[1;33m'
    BLUE='\033[0;34m'
    NC='\033[0m'
else
    RED=''; GREEN=''; YELLOW=''; BLUE=''; NC=''
fi

# Configuration
LOG_DIR="logs"
BIN_DIR="bin"
RUN1_DIR="${BIN_DIR}_run1"
RUN2_DIR="${BIN_DIR}_run2"

# Create log directory
mkdir -p "$LOG_DIR"

echo "${BLUE}==================================================${NC}"
echo "${BLUE}  GlyphOS Determinism Parity Check${NC}"
echo "${BLUE}==================================================${NC}"
echo ""

# Set deterministic environment
echo "${YELLOW}Setting deterministic environment...${NC}"
export TZ=UTC
export LANG=C
export LC_ALL=C
export SOURCE_DATE_EPOCH=1701820800
export GDF_SEED=0

echo "  TZ=$TZ"
echo "  LANG=$LANG"
echo "  LC_ALL=$LC_ALL"
echo "  SOURCE_DATE_EPOCH=$SOURCE_DATE_EPOCH"
echo "  GDF_SEED=$GDF_SEED"
echo ""

# Compiler info
echo "${YELLOW}Compiler information:${NC}"
export CC="${CC:-gcc}"
$CC --version | head -1
echo ""

# ============================================
# RUN 1
# ============================================

echo "${BLUE}==================================================${NC}"
echo "${BLUE}  Build Run #1${NC}"
echo "${BLUE}==================================================${NC}"
echo ""

# Clean workspace
echo "${YELLOW}Cleaning workspace...${NC}"
if [ -d "$BIN_DIR" ]; then
    mv "$BIN_DIR" "${BIN_DIR}_backup_$(date +%s)" 2>/dev/null || rm -rf "$BIN_DIR"
fi
mkdir -p "$BIN_DIR"

# Build
echo "${YELLOW}Building components...${NC}"
if [ -x "scripts/unified_pipeline.sh" ]; then
    if ! sh scripts/unified_pipeline.sh --clean --ci > "$LOG_DIR/run1_build.log" 2>&1; then
        echo "${RED}✗ Build run #1 FAILED${NC}"
        tail -30 "$LOG_DIR/run1_build.log"
        exit 2
    fi
else
    # Fallback: build manually
    $CC -O2 -Wall -Wextra -o "$BIN_DIR/substrate_core" src/substrate_core.c -lm > "$LOG_DIR/run1_substrate.log" 2>&1
    $CC -O2 -Wall -Wextra -o "$BIN_DIR/glyph_interp" src/glyph_interpreter.c -lm > "$LOG_DIR/run1_glyph.log" 2>&1
fi

echo "${GREEN}✓ Build run #1 complete${NC}"

# Save binaries
echo "${YELLOW}Saving binaries from run #1...${NC}"
mkdir -p "$RUN1_DIR"
cp -p "$BIN_DIR"/* "$RUN1_DIR"/ 2>/dev/null || true
ls -lh "$RUN1_DIR"
echo ""

# Compute checksums
echo "${YELLOW}Computing checksums for run #1...${NC}"
(cd "$RUN1_DIR" && sha256sum * > "../$LOG_DIR/run1_checksums.txt" 2>/dev/null || true)
cat "$LOG_DIR/run1_checksums.txt"
echo ""

# ============================================
# RUN 2
# ============================================

echo "${BLUE}==================================================${NC}"
echo "${BLUE}  Build Run #2${NC}"
echo "${BLUE}==================================================${NC}"
echo ""

# Clean workspace again
echo "${YELLOW}Cleaning workspace...${NC}"
rm -rf "$BIN_DIR"
mkdir -p "$BIN_DIR"

# Wait a moment to ensure different timestamps if non-deterministic
sleep 1

# Build again
echo "${YELLOW}Building components...${NC}"
if [ -x "scripts/unified_pipeline.sh" ]; then
    if ! sh scripts/unified_pipeline.sh --clean --ci > "$LOG_DIR/run2_build.log" 2>&1; then
        echo "${RED}✗ Build run #2 FAILED${NC}"
        tail -30 "$LOG_DIR/run2_build.log"
        exit 2
    fi
else
    # Fallback: build manually
    $CC -O2 -Wall -Wextra -o "$BIN_DIR/substrate_core" src/substrate_core.c -lm > "$LOG_DIR/run2_substrate.log" 2>&1
    $CC -O2 -Wall -Wextra -o "$BIN_DIR/glyph_interp" src/glyph_interpreter.c -lm > "$LOG_DIR/run2_glyph.log" 2>&1
fi

echo "${GREEN}✓ Build run #2 complete${NC}"

# Save binaries
echo "${YELLOW}Saving binaries from run #2...${NC}"
mkdir -p "$RUN2_DIR"
cp -p "$BIN_DIR"/* "$RUN2_DIR"/ 2>/dev/null || true
ls -lh "$RUN2_DIR"
echo ""

# Compute checksums
echo "${YELLOW}Computing checksums for run #2...${NC}"
(cd "$RUN2_DIR" && sha256sum * > "../$LOG_DIR/run2_checksums.txt" 2>/dev/null || true)
cat "$LOG_DIR/run2_checksums.txt"
echo ""

# ============================================
# COMPARISON
# ============================================

echo "${BLUE}==================================================${NC}"
echo "${BLUE}  Comparing Builds${NC}"
echo "${BLUE}==================================================${NC}"
echo ""

# Compare files
MISMATCH=0

for file in "$RUN1_DIR"/*; do
    basename=$(basename "$file")
    file2="$RUN2_DIR/$basename"

    if [ ! -f "$file2" ]; then
        echo "${RED}✗ File missing in run #2: $basename${NC}"
        MISMATCH=1
        continue
    fi

    # Compare checksums
    sum1=$(sha256sum "$file" | awk '{print $1}')
    sum2=$(sha256sum "$file2" | awk '{print $1}')

    if [ "$sum1" = "$sum2" ]; then
        echo "${GREEN}✓ $basename: IDENTICAL${NC}"
        echo "  SHA256: $sum1"
    else
        echo "${RED}✗ $basename: DIFFERENT${NC}"
        echo "  Run #1: $sum1"
        echo "  Run #2: $sum2"
        MISMATCH=1
    fi
done

echo ""

# Final verdict
echo "${BLUE}==================================================${NC}"
echo "${BLUE}  Determinism Verdict${NC}"
echo "${BLUE}==================================================${NC}"
echo ""

if [ $MISMATCH -eq 0 ]; then
    echo "${GREEN}✅ DETERMINISTIC BUILD VERIFIED${NC}"
    echo ""
    echo "All binaries are bit-identical across independent builds."
    echo "This confirms that the build process is reproducible and"
    echo "deterministic when given the same source and environment."
    echo ""
    exit 0
else
    echo "${RED}❌ NON-DETERMINISTIC BUILD DETECTED${NC}"
    echo ""
    echo "Some binaries differ between runs. This indicates that"
    echo "the build process has non-deterministic elements such as:"
    echo "  - Timestamps embedded in binaries"
    echo "  - Random number generation"
    echo "  - Memory address randomization in debug info"
    echo "  - Uninitialized memory reads"
    echo ""
    echo "Check build logs in logs/ for details."
    echo ""
    exit 1
fi
