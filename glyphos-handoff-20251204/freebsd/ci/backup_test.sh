#!/bin/sh
#
# GlyphOS Backup and Recovery Test
# Version: 1.0.0
#
# Tests backup and restore procedures for vault/ and logs/
#
# Usage: ./ci/backup_test.sh
#
# Exit codes:
#   0 - Backup and restore successful
#   1 - Backup or restore failed
#   2 - Checksum mismatch after restore
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
VAULT_DIR="vault"
LOGS_DIR="logs"
BACKUP_DIR="backups"
TIMESTAMP=$(date -u +%Y%m%d_%H%M%S)

# Create backup directory
mkdir -p "$BACKUP_DIR"

echo "${BLUE}==================================================${NC}"
echo "${BLUE}  GlyphOS Backup and Recovery Test${NC}"
echo "${BLUE}==================================================${NC}"
echo ""

# ============================================
# STEP 1: Create Checksums
# ============================================

echo "${BLUE}[1/6] Computing original checksums...${NC}"

if [ ! -d "$VAULT_DIR" ] || [ -z "$(ls -A $VAULT_DIR 2>/dev/null)" ]; then
    echo "${RED}✗ Vault directory is empty or missing${NC}"
    exit 1
fi

# Save checksums
(cd "$VAULT_DIR" && find . -type f -exec sha256sum {} \; | sort > "../$BACKUP_DIR/vault_checksums_orig.txt")

VAULT_FILE_COUNT=$(wc -l < "$BACKUP_DIR/vault_checksums_orig.txt" | tr -d ' ')
echo "${GREEN}✓ Computed checksums for $VAULT_FILE_COUNT files${NC}"
echo ""

# ============================================
# STEP 2: Create Backup
# ============================================

echo "${BLUE}[2/6] Creating backup archives...${NC}"

# Backup vault
VAULT_BACKUP="$BACKUP_DIR/vault_backup_${TIMESTAMP}.tar.gz"
tar -czf "$VAULT_BACKUP" "$VAULT_DIR" 2>/dev/null
VAULT_BACKUP_SIZE=$(du -h "$VAULT_BACKUP" | cut -f1)
echo "${GREEN}✓ Vault backup: $VAULT_BACKUP ($VAULT_BACKUP_SIZE)${NC}"

# Backup logs (if exists)
if [ -d "$LOGS_DIR" ] && [ -n "$(ls -A $LOGS_DIR 2>/dev/null)" ]; then
    LOGS_BACKUP="$BACKUP_DIR/logs_backup_${TIMESTAMP}.tar.gz"
    tar -czf "$LOGS_BACKUP" "$LOGS_DIR" 2>/dev/null
    LOGS_BACKUP_SIZE=$(du -h "$LOGS_BACKUP" | cut -f1)
    echo "${GREEN}✓ Logs backup: $LOGS_BACKUP ($LOGS_BACKUP_SIZE)${NC}"
else
    echo "${YELLOW}⚠ Logs directory empty, skipping backup${NC}"
fi

echo ""

# ============================================
# STEP 3: Test Glyph Listing (Pre-Delete)
# ============================================

echo "${BLUE}[3/6] Testing glyph listing before deletion...${NC}"

if [ -f "bin/glyph_interp" ]; then
    GLYPH_COUNT=$(./bin/glyph_interp --vault "$VAULT_DIR" --list 2>/dev/null | grep -c "glyph_id:" || echo "0")
    echo "${GREEN}✓ Found $GLYPH_COUNT glyphs in vault${NC}"
else
    echo "${YELLOW}⚠ glyph_interp not found, skipping listing test${NC}"
    GLYPH_COUNT="N/A"
fi

echo ""

# ============================================
# STEP 4: Delete Vault
# ============================================

echo "${BLUE}[4/6] Deleting vault (simulating data loss)...${NC}"

# Move vault to temporary location (safer than rm -rf)
VAULT_TEMP="${VAULT_DIR}_deleted_${TIMESTAMP}"
mv "$VAULT_DIR" "$VAULT_TEMP"

if [ -d "$VAULT_DIR" ]; then
    echo "${RED}✗ Failed to delete vault${NC}"
    exit 1
fi

echo "${GREEN}✓ Vault deleted (moved to $VAULT_TEMP)${NC}"
echo ""

# ============================================
# STEP 5: Restore from Backup
# ============================================

echo "${BLUE}[5/6] Restoring from backup...${NC}"

# Extract vault backup
tar -xzf "$VAULT_BACKUP" 2>/dev/null

if [ ! -d "$VAULT_DIR" ]; then
    echo "${RED}✗ Restore failed: vault directory not recreated${NC}"
    exit 1
fi

echo "${GREEN}✓ Vault restored from $VAULT_BACKUP${NC}"

# Compute new checksums
(cd "$VAULT_DIR" && find . -type f -exec sha256sum {} \; | sort > "../$BACKUP_DIR/vault_checksums_restored.txt")

echo ""

# ============================================
# STEP 6: Verify Integrity
# ============================================

echo "${BLUE}[6/6] Verifying restore integrity...${NC}"

# Compare checksums
if diff -q "$BACKUP_DIR/vault_checksums_orig.txt" "$BACKUP_DIR/vault_checksums_restored.txt" > /dev/null 2>&1; then
    echo "${GREEN}✓ Checksums match - restore successful${NC}"
else
    echo "${RED}✗ Checksum mismatch detected${NC}"
    echo ""
    echo "Differences:"
    diff "$BACKUP_DIR/vault_checksums_orig.txt" "$BACKUP_DIR/vault_checksums_restored.txt" || true
    exit 2
fi

# Test glyph listing again
if [ -f "bin/glyph_interp" ]; then
    GLYPH_COUNT_RESTORED=$(./bin/glyph_interp --vault "$VAULT_DIR" --list 2>/dev/null | grep -c "glyph_id:" || echo "0")

    if [ "$GLYPH_COUNT" = "$GLYPH_COUNT_RESTORED" ]; then
        echo "${GREEN}✓ Glyph count matches: $GLYPH_COUNT_RESTORED glyphs${NC}"
    else
        echo "${RED}✗ Glyph count mismatch: expected $GLYPH_COUNT, got $GLYPH_COUNT_RESTORED${NC}"
        exit 2
    fi
fi

echo ""

# ============================================
# Summary
# ============================================

echo "${BLUE}==================================================${NC}"
echo "${BLUE}  Backup and Recovery Summary${NC}"
echo "${BLUE}==================================================${NC}"
echo ""
echo "Original files:   $VAULT_FILE_COUNT"
echo "Backup size:      $VAULT_BACKUP_SIZE"
echo "Restored files:   $(wc -l < $BACKUP_DIR/vault_checksums_restored.txt | tr -d ' ')"
echo "Checksum status:  ${GREEN}VERIFIED${NC}"

if [ "$GLYPH_COUNT" != "N/A" ]; then
    echo "Glyph count:      $GLYPH_COUNT_RESTORED (verified)"
fi

echo ""
echo "${GREEN}✅ BACKUP AND RECOVERY TEST PASSED${NC}"
echo ""
echo "Backup files:"
echo "  - $VAULT_BACKUP"
if [ -n "$LOGS_BACKUP" ]; then
    echo "  - $LOGS_BACKUP"
fi
echo ""
echo "To restore manually:"
echo "  tar -xzf $VAULT_BACKUP"
echo ""

# Cleanup temporary directory
echo "${YELLOW}Cleaning up temporary files...${NC}"
rm -rf "$VAULT_TEMP"
echo "${GREEN}✓ Cleanup complete${NC}"
echo ""

exit 0
