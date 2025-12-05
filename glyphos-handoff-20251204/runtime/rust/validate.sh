#!/bin/bash
# Validation script for Glyph OS Rust components

set -e

echo "=========================================="
echo "Glyph OS Rust Runtime Validation"
echo "=========================================="
echo ""

# Check if Rust is installed
if ! command -v cargo &> /dev/null; then
    echo "ERROR: Rust/Cargo not found!"
    echo "Please install Rust from https://rustup.rs/"
    echo ""
    echo "Run: curl --proto '=https' --tlsv1.2 -sSf https://sh.rustup.rs | sh"
    exit 1
fi

echo "✓ Rust toolchain found: $(rustc --version)"
echo ""

# Validate file structure
echo "Validating file structure..."
echo ""

required_files=(
    "glyphd/Cargo.toml"
    "glyphd/src/main.rs"
    "glyph-spu/Cargo.toml"
    "glyph-spu/src/main.rs"
    "Cargo.toml"
    "README.md"
)

all_found=true
for file in "${required_files[@]}"; do
    if [ -f "$file" ]; then
        echo "  ✓ $file"
    else
        echo "  ✗ $file (MISSING)"
        all_found=false
    fi
done

if [ "$all_found" = false ]; then
    echo ""
    echo "ERROR: Some required files are missing!"
    exit 1
fi

echo ""
echo "✓ All required files present"
echo ""

# Build workspace
echo "Building workspace..."
echo ""
cargo build --workspace

echo ""
echo "✓ Workspace build successful"
echo ""

# Run tests
echo "Running tests..."
echo ""
cargo test --workspace --quiet

echo ""
echo "✓ All tests passed"
echo ""

# Build release binaries
echo "Building release binaries..."
echo ""
cargo build --workspace --release

echo ""
echo "✓ Release build successful"
echo ""

# Check binary sizes
echo "Binary information:"
echo ""
if [ -f "target/release/glyphd" ]; then
    glyphd_size=$(du -h target/release/glyphd | cut -f1)
    echo "  glyphd:    $glyphd_size"
fi
if [ -f "target/release/glyph-spu" ]; then
    spu_size=$(du -h target/release/glyph-spu | cut -f1)
    echo "  glyph-spu: $spu_size"
fi

echo ""
echo "=========================================="
echo "✓ All validation checks passed!"
echo "=========================================="
echo ""
echo "Next steps:"
echo "  1. Run glyphd:    cargo run --release --bin glyphd"
echo "  2. Run glyph-spu: cargo run --release --bin glyph-spu"
echo ""
echo "API endpoints:"
echo "  glyphd:    http://localhost:8080"
echo "  glyph-spu: http://localhost:8081"
echo ""
