#!/bin/sh
# GlyphOS Phase 4 Glyph Interpreter Demo

echo "==================================="
echo "GlyphOS Phase 4 Demonstration"
echo "==================================="
echo ""

echo "1. Running comprehensive test suite..."
./bin/glyph_interp --test | head -50
echo ""

echo "2. Loading GDF files from vault..."
./bin/glyph_interp --vault ./vault --list
echo ""

echo "3. Activating glyph with inheritance chain..."
./bin/glyph_interp --vault ./vault --activate 001
echo ""

echo "Demo complete!"
