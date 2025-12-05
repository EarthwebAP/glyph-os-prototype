#!/bin/sh
#
# GlyphOS Substrate Core Build Script
#
# This script builds and tests the Substrate Core implementation
#
# Usage: ./build_substrate.sh [--clean] [--test]
#

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Directories
SRC_DIR="src"
BIN_DIR="bin"
SUBSTRATE_SRC="$SRC_DIR/substrate_core.c"
SUBSTRATE_BIN="$BIN_DIR/substrate_core"

# Compiler settings
CC="${CC:-cc}"
CFLAGS="-Wall -Wextra -O2"
LDFLAGS="-lm"

# Parse arguments
DO_CLEAN=0
DO_TEST=0

for arg in "$@"; do
    case "$arg" in
        --clean)
            DO_CLEAN=1
            ;;
        --test)
            DO_TEST=1
            ;;
        --help)
            echo "Usage: $0 [OPTIONS]"
            echo ""
            echo "Options:"
            echo "  --clean    Clean build artifacts before building"
            echo "  --test     Run test suite after building"
            echo "  --help     Display this help message"
            echo ""
            echo "Examples:"
            echo "  $0                  # Build only"
            echo "  $0 --test           # Build and test"
            echo "  $0 --clean --test   # Clean, build, and test"
            exit 0
            ;;
        *)
            echo "Unknown option: $arg"
            echo "Use --help for usage information"
            exit 1
            ;;
    esac
done

# Clean if requested
if [ $DO_CLEAN -eq 1 ]; then
    echo "${YELLOW}Cleaning build artifacts...${NC}"
    rm -f "$SUBSTRATE_BIN"
    echo "${GREEN}Clean complete${NC}"
    echo ""
fi

# Create bin directory if needed
if [ ! -d "$BIN_DIR" ]; then
    mkdir -p "$BIN_DIR"
fi

# Build
echo "${YELLOW}Building Substrate Core...${NC}"
echo "Source: $SUBSTRATE_SRC"
echo "Output: $SUBSTRATE_BIN"
echo "Compiler: $CC"
echo "Flags: $CFLAGS $LDFLAGS"
echo ""

if ! $CC $CFLAGS -o "$SUBSTRATE_BIN" "$SUBSTRATE_SRC" $LDFLAGS; then
    echo "${RED}Build failed!${NC}"
    exit 1
fi

echo "${GREEN}Build successful!${NC}"
echo ""

# Show binary info
echo "Binary information:"
ls -lh "$SUBSTRATE_BIN"
echo ""

# Test if requested
if [ $DO_TEST -eq 1 ]; then
    echo "${YELLOW}Running test suite...${NC}"
    echo ""

    if ! "$SUBSTRATE_BIN" --test; then
        echo ""
        echo "${RED}Tests failed!${NC}"
        exit 1
    fi

    echo ""
    echo "${GREEN}All tests passed!${NC}"
    echo ""

    # Show status
    echo "${YELLOW}Substrate status:${NC}"
    echo ""
    "$SUBSTRATE_BIN" --status
fi

echo "${GREEN}Done!${NC}"
