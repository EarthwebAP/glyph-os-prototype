#!/bin/sh
#
# Collect CI Artifacts Script
# Downloads all artifacts from the latest CI run for auditing
#

set -e

BRANCH="${1:-feature/release-readiness}"
OUTPUT_DIR="${2:-ci-artifacts-$(date +%Y%m%d-%H%M%S)}"

echo "=== GlyphOS CI Artifact Collection ==="
echo "Branch: $BRANCH"
echo "Output: $OUTPUT_DIR"
echo ""

mkdir -p "$OUTPUT_DIR"

# Note: Requires GitHub CLI (gh) to be installed and authenticated
if ! command -v gh > /dev/null 2>&1; then
    echo "ERROR: GitHub CLI (gh) not installed"
    echo "Install: https://cli.github.com/"
    echo ""
    echo "Alternative: Download artifacts manually from:"
    echo "https://github.com/EarthwebAP/glyph-os-prototype/actions"
    exit 1
fi

# Get latest run ID for the branch
echo "[1/7] Finding latest CI run..."
RUN_ID=$(gh run list --branch "$BRANCH" --workflow "ci.yml" --limit 1 --json databaseId --jq '.[0].databaseId')

if [ -z "$RUN_ID" ]; then
    echo "ERROR: No CI runs found for branch $BRANCH"
    exit 1
fi

echo "Found run ID: $RUN_ID"
echo ""

# Download all artifacts
echo "[2/7] Downloading artifacts..."
gh run download "$RUN_ID" --dir "$OUTPUT_DIR"

echo ""
echo "[3/7] Organizing artifacts..."

# Create structured directories
mkdir -p "$OUTPUT_DIR/checksums"
mkdir -p "$OUTPUT_DIR/sanitizer-logs"
mkdir -p "$OUTPUT_DIR/determinism-logs"
mkdir -p "$OUTPUT_DIR/test-logs"
mkdir -p "$OUTPUT_DIR/release"
mkdir -p "$OUTPUT_DIR/fuzzing"
mkdir -p "$OUTPUT_DIR/security"

# Move artifacts to appropriate directories
if [ -d "$OUTPUT_DIR/glyphos-release-artifacts" ]; then
    echo "  - Organizing release artifacts..."
    mv "$OUTPUT_DIR/glyphos-release-artifacts/artifacts/checksums.sha256" "$OUTPUT_DIR/checksums/" 2>/dev/null || true
    mv "$OUTPUT_DIR/glyphos-release-artifacts/artifacts/release_manifest.json" "$OUTPUT_DIR/release/release_manifest.glyphos-node-alpha.json" 2>/dev/null || true
    mv "$OUTPUT_DIR/glyphos-release-artifacts/bin/"* "$OUTPUT_DIR/release/" 2>/dev/null || true
    mv "$OUTPUT_DIR/glyphos-release-artifacts/artifacts/"*.sig "$OUTPUT_DIR/release/" 2>/dev/null || true
    mv "$OUTPUT_DIR/glyphos-release-artifacts/artifacts/"*.asc "$OUTPUT_DIR/release/" 2>/dev/null || true
fi

if [ -d "$OUTPUT_DIR/sanitizer-logs-address" ]; then
    echo "  - Organizing sanitizer logs..."
    mv "$OUTPUT_DIR/sanitizer-logs-"*/*.log "$OUTPUT_DIR/sanitizer-logs/" 2>/dev/null || true
fi

if [ -d "$OUTPUT_DIR/parity-logs" ]; then
    echo "  - Organizing determinism logs..."
    mv "$OUTPUT_DIR/parity-logs"/* "$OUTPUT_DIR/determinism-logs/" 2>/dev/null || true
fi

if [ -d "$OUTPUT_DIR/test-logs-gcc" ] || [ -d "$OUTPUT_DIR/test-logs-clang" ]; then
    echo "  - Organizing test logs..."
    mv "$OUTPUT_DIR/test-logs-"*/*.log "$OUTPUT_DIR/test-logs/" 2>/dev/null || true
fi

if [ -d "$OUTPUT_DIR/fuzzing-artifacts" ]; then
    echo "  - Organizing fuzzing artifacts..."
    mv "$OUTPUT_DIR/fuzzing-artifacts"/* "$OUTPUT_DIR/fuzzing/" 2>/dev/null || true
fi

# Collect CI metadata
echo ""
echo "[4/7] Collecting CI metadata..."
gh run view "$RUN_ID" --json conclusion,createdAt,headBranch,headSha,url,workflowName > "$OUTPUT_DIR/ci-run-metadata.json"

# Generate artifact inventory
echo ""
echo "[5/7] Generating artifact inventory..."
cat > "$OUTPUT_DIR/INVENTORY.md" << 'EOF'
# GlyphOS CI Artifact Inventory

**Collection Date**: $(date -u +%Y-%m-%d_%H:%M:%S_UTC)
**Branch**: $BRANCH
**Run ID**: $RUN_ID

## Artifact Categories

### Release Artifacts
- `release/substrate_core` - Production substrate binary
- `release/glyph_interp` - Production interpreter binary
- `release/release_manifest.glyphos-node-alpha.json` - Release manifest
- `checksums/checksums.sha256` - SHA256 checksums for verification
- `release/*.sig` - Cosign signatures
- `release/*.asc` - GPG signatures (if available)

### Security Testing
- `sanitizer-logs/address_*.log` - AddressSanitizer results
- `sanitizer-logs/undefined_*.log` - UndefinedBehaviorSanitizer results
- `sanitizer-logs/memory_*.log` - MemorySanitizer results
- `security/` - Security scan results (if available)

### Determinism Verification
- `determinism-logs/run1_substrate.txt` - First build test output
- `determinism-logs/run2_substrate.txt` - Second build test output
- `determinism-logs/run1_glyph.txt` - First build interpreter output
- `determinism-logs/run2_glyph.txt` - Second build interpreter output

### Test Results
- `test-logs/substrate_test.log` - Substrate core tests
- `test-logs/glyph_test.log` - Glyph interpreter tests

### Fuzzing
- `fuzzing/corpus/` - Fuzzing corpus files
- `fuzzing/crash-*` - Crash reports (if any found)

### Metadata
- `ci-run-metadata.json` - CI run information
- `INVENTORY.md` - This file

## Verification

Verify release binaries:
```bash
cd release/
sha256sum -c ../checksums/checksums.sha256
```

Verify Cosign signatures:
```bash
cosign verify-blob --signature substrate_core.sig substrate_core
cosign verify-blob --signature glyph_interp.sig glyph_interp
```

## Audit Package

For external security audit, provide:
- All files in `release/`
- All files in `sanitizer-logs/`
- All files in `determinism-logs/`
- All files in `fuzzing/`
- Source code snapshot (git archive)
- Threat model documentation
- Security patches documentation
EOF

# Substitute variables in INVENTORY.md
sed -i.bak \
    -e "s|\$(date -u +%Y-%m-%d_%H:%M:%S_UTC)|$(date -u +%Y-%m-%d_%H:%M:%S_UTC)|g" \
    -e "s|\$BRANCH|$BRANCH|g" \
    -e "s|\$RUN_ID|$RUN_ID|g" \
    "$OUTPUT_DIR/INVENTORY.md"
rm "$OUTPUT_DIR/INVENTORY.md.bak"

# Generate checksums for all collected artifacts
echo ""
echo "[6/7] Generating checksums..."
find "$OUTPUT_DIR" -type f -not -name "*.sha256" -exec sha256sum {} \; > "$OUTPUT_DIR/artifact-checksums.sha256"

# Create tarball
echo ""
echo "[7/7] Creating archive..."
TARBALL="glyphos-ci-artifacts-$(date +%Y%m%d-%H%M%S).tar.gz"
tar -czf "$TARBALL" "$OUTPUT_DIR"

echo ""
echo "=== Collection Complete ==="
echo ""
echo "Artifacts saved to: $OUTPUT_DIR/"
echo "Archive created: $TARBALL"
echo ""
echo "Next steps:"
echo "  1. Review INVENTORY.md"
echo "  2. Verify checksums"
echo "  3. Package for auditor (see scripts/package_auditor_bundle.sh)"
echo ""
