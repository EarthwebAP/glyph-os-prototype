#!/bin/sh
#
# GlyphOS Unified Build Pipeline
# Version: 1.0.0
#
# Builds all GlyphOS components with optional sanitizer support
#
# Usage: ./unified_pipeline.sh [OPTIONS]
#
# Options:
#   --clean        Clean build artifacts before building
#   --test         Run tests after building
#   --sanitizer    Build with AddressSanitizer and UndefinedBehaviorSanitizer
#   --ci           CI mode (deterministic, verbose)
#   --help         Show this help message
#
# Examples:
#   ./unified_pipeline.sh --clean --test
#   ./unified_pipeline.sh --sanitizer --test
#   CC=clang CFLAGS="-fsanitize=address,undefined -O1 -g" ./unified_pipeline.sh --sanitizer
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

# Directories
SRC_DIR="src"
BIN_DIR="bin"
LOG_DIR="logs"

# Components
COMPONENTS="substrate_core glyph_interp"

# Compiler settings
CC="${CC:-clang}"
BASE_CFLAGS="-Wall -Wextra"
OPT_LEVEL="${OPT_LEVEL:--O2}"
LDFLAGS="-lm"

# Parse arguments
DO_CLEAN=0
DO_TEST=0
DO_SANITIZER=0
CI_MODE=0

for arg in "$@"; do
    case "$arg" in
        --clean)
            DO_CLEAN=1
            ;;
        --test)
            DO_TEST=1
            ;;
        --sanitizer)
            DO_SANITIZER=1
            ;;
        --ci)
            CI_MODE=1
            ;;
        --help)
            sed -n '2,20p' "$0" | sed 's/^# //'
            exit 0
            ;;
        *)
            echo "${RED}Unknown option: $arg${NC}" >&2
            echo "Use --help for usage information" >&2
            exit 1
            ;;
    esac
done

# CI mode settings
if [ $CI_MODE -eq 1 ]; then
    export TZ=UTC
    export LANG=C
    export LC_ALL=C
    export SOURCE_DATE_EPOCH=1701820800
    export GDF_SEED=0
    echo "${BLUE}=== CI MODE ENABLED ===${NC}"
    echo "TZ=$TZ, LANG=$LANG, SOURCE_DATE_EPOCH=$SOURCE_DATE_EPOCH"
fi

# Create directories
mkdir -p "$BIN_DIR" "$LOG_DIR"

# Clean if requested
if [ $DO_CLEAN -eq 1 ]; then
    echo "${YELLOW}Cleaning build artifacts...${NC}"
    rm -rf "$BIN_DIR"/* "$LOG_DIR"/*
    echo "${GREEN}✓ Clean complete${NC}"
fi

# Determine CFLAGS
if [ $DO_SANITIZER -eq 1 ]; then
    # Sanitizer build
    if [ -z "$CFLAGS" ]; then
        CFLAGS="$BASE_CFLAGS -fsanitize=address,undefined -O1 -g -fno-omit-frame-pointer"
    fi
    SUFFIX="_san"
    echo "${YELLOW}=== SANITIZER BUILD ===${NC}"
    echo "CFLAGS: $CFLAGS"
else
    # Normal build
    if [ -z "$CFLAGS" ]; then
        CFLAGS="$BASE_CFLAGS $OPT_LEVEL"
    fi
    SUFFIX=""
fi

# Build counter
BUILD_SUCCESS=0
BUILD_TOTAL=0
TEST_SUCCESS=0
TEST_TOTAL=0

echo ""
echo "${BLUE}==================================================${NC}"
echo "${BLUE}  GlyphOS Unified Build Pipeline${NC}"
echo "${BLUE}==================================================${NC}"
echo ""
echo "Compiler:  $CC"
echo "CFLAGS:    $CFLAGS"
echo "LDFLAGS:   $LDFLAGS"
echo "Sanitizer: $([ $DO_SANITIZER -eq 1 ] && echo 'ENABLED' || echo 'DISABLED')"
echo ""

# Build substrate_core
echo "${BLUE}[1/2] Building substrate_core...${NC}"
BUILD_TOTAL=$((BUILD_TOTAL + 1))

if $CC $CFLAGS -o "$BIN_DIR/substrate_core$SUFFIX" "$SRC_DIR/substrate_core.c" $LDFLAGS > "$LOG_DIR/substrate_core_build.log" 2>&1; then
    echo "${GREEN}  ✓ substrate_core built successfully${NC}"
    ls -lh "$BIN_DIR/substrate_core$SUFFIX"
    BUILD_SUCCESS=$((BUILD_SUCCESS + 1))
else
    echo "${RED}  ✗ substrate_core build FAILED${NC}"
    echo "  See logs/substrate_core_build.log for details"
    tail -20 "$LOG_DIR/substrate_core_build.log"
    exit 1
fi

# Build glyph_interpreter
echo ""
echo "${BLUE}[2/2] Building glyph_interpreter...${NC}"
BUILD_TOTAL=$((BUILD_TOTAL + 1))

if $CC $CFLAGS -o "$BIN_DIR/glyph_interp$SUFFIX" "$SRC_DIR/glyph_interpreter.c" $LDFLAGS > "$LOG_DIR/glyph_interp_build.log" 2>&1; then
    echo "${GREEN}  ✓ glyph_interp built successfully${NC}"
    ls -lh "$BIN_DIR/glyph_interp$SUFFIX"
    BUILD_SUCCESS=$((BUILD_SUCCESS + 1))
else
    echo "${RED}  ✗ glyph_interp build FAILED${NC}"
    echo "  See logs/glyph_interp_build.log for details"
    tail -20 "$LOG_DIR/glyph_interp_build.log"
    exit 1
fi

# Run tests if requested
if [ $DO_TEST -eq 1 ]; then
    echo ""
    echo "${BLUE}==================================================${NC}"
    echo "${BLUE}  Running Test Suite${NC}"
    echo "${BLUE}==================================================${NC}"
    echo ""

    # Test substrate_core
    echo "${BLUE}[1/2] Testing substrate_core...${NC}"
    TEST_TOTAL=$((TEST_TOTAL + 1))

    if "$BIN_DIR/substrate_core$SUFFIX" --test > "$LOG_DIR/substrate_core_test.log" 2>&1; then
        echo "${GREEN}  ✓ substrate_core tests PASSED${NC}"
        grep "Test.*:" "$LOG_DIR/substrate_core_test.log" | head -10
        TEST_SUCCESS=$((TEST_SUCCESS + 1))
    else
        echo "${RED}  ✗ substrate_core tests FAILED${NC}"
        echo "  See logs/substrate_core_test.log for details"
        tail -30 "$LOG_DIR/substrate_core_test.log"
        exit 1
    fi

    # Test glyph_interpreter
    echo ""
    echo "${BLUE}[2/2] Testing glyph_interpreter...${NC}"
    TEST_TOTAL=$((TEST_TOTAL + 1))

    if "$BIN_DIR/glyph_interp$SUFFIX" --test > "$LOG_DIR/glyph_interp_test.log" 2>&1; then
        echo "${GREEN}  ✓ glyph_interp tests PASSED${NC}"
        grep "Test.*:" "$LOG_DIR/glyph_interp_test.log" | head -10
        TEST_SUCCESS=$((TEST_SUCCESS + 1))
    else
        echo "${RED}  ✗ glyph_interp tests FAILED${NC}"
        echo "  See logs/glyph_interp_test.log for details"
        tail -30 "$LOG_DIR/glyph_interp_test.log"
        exit 1
    fi
fi

# Summary
echo ""
echo "${BLUE}==================================================${NC}"
echo "${BLUE}  Build Summary${NC}"
echo "${BLUE}==================================================${NC}"
echo ""
echo "Builds:  ${GREEN}$BUILD_SUCCESS${NC}/$BUILD_TOTAL passed"

if [ $DO_TEST -eq 1 ]; then
    echo "Tests:   ${GREEN}$TEST_SUCCESS${NC}/$TEST_TOTAL passed"
fi

echo ""

if [ $BUILD_SUCCESS -eq $BUILD_TOTAL ] && ([ $DO_TEST -eq 0 ] || [ $TEST_SUCCESS -eq $TEST_TOTAL ]); then
    echo "${GREEN}✅ ALL CHECKS PASSED${NC}"
    echo ""
    exit 0
else
    echo "${RED}❌ SOME CHECKS FAILED${NC}"
    echo ""
    exit 1
fi
