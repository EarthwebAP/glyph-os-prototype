#!/bin/sh
#
# GlyphOS Artifact Signing Script
#
# Signs release artifacts (binaries, ISO, manifests) with multiple methods:
#   1. GPG/RSA detached signatures
#   2. Cosign keyless signatures (Sigstore)
#   3. SHA256 checksums
#
# Usage:
#   ./sign_artifacts.sh [OPTIONS]
#
# Options:
#   --gpg           Use GPG signing (requires GPG_SIGNING_KEY or local key)
#   --cosign        Use Cosign keyless signing (requires OIDC)
#   --kms PROVIDER  Use cloud KMS (aws|gcp|azure)
#   --artifacts DIR Directory containing artifacts to sign (default: artifacts/)
#   --output DIR    Output directory for signatures (default: same as artifacts)
#   --manifest FILE Release manifest file (default: release_manifest.glyphos-node-alpha.json)
#   --dry-run       Show what would be signed without signing
#
# Environment Variables:
#   GPG_SIGNING_KEY       GPG private key (armored or base64)
#   GPG_PASSPHRASE        GPG key passphrase (optional)
#   AWS_ACCESS_KEY_ID     AWS credentials (for KMS)
#   AWS_KMS_KEY_ID        AWS KMS key ID
#   GCP_SA_KEY            GCP service account key JSON
#   COSIGN_EXPERIMENTAL   Set to 1 for keyless signing

set -e

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Configuration
ARTIFACTS_DIR="artifacts"
OUTPUT_DIR=""
MANIFEST_FILE="release_manifest.glyphos-node-alpha.json"
USE_GPG=0
USE_COSIGN=0
USE_KMS=""
DRY_RUN=0

# Parse arguments
while [ $# -gt 0 ]; do
    case "$1" in
        --gpg)
            USE_GPG=1
            shift
            ;;
        --cosign)
            USE_COSIGN=1
            shift
            ;;
        --kms)
            USE_KMS="$2"
            shift 2
            ;;
        --artifacts)
            ARTIFACTS_DIR="$2"
            shift 2
            ;;
        --output)
            OUTPUT_DIR="$2"
            shift 2
            ;;
        --manifest)
            MANIFEST_FILE="$2"
            shift 2
            ;;
        --dry-run)
            DRY_RUN=1
            shift
            ;;
        *)
            echo "${RED}Unknown option: $1${NC}" >&2
            exit 1
            ;;
    esac
done

# Default output dir to artifacts dir
if [ -z "$OUTPUT_DIR" ]; then
    OUTPUT_DIR="$ARTIFACTS_DIR"
fi

echo "${BLUE}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"
echo "${BLUE}  GlyphOS Artifact Signing${NC}"
echo "${BLUE}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"
echo ""

# Check if artifacts directory exists
if [ ! -d "$ARTIFACTS_DIR" ]; then
    echo "${RED}✗ Artifacts directory not found: $ARTIFACTS_DIR${NC}" >&2
    exit 1
fi

# Create output directory
mkdir -p "$OUTPUT_DIR"

# Find artifacts to sign
ARTIFACT_FILES=$(find "$ARTIFACTS_DIR" -type f \( -name "*.iso" -o -name "substrate_core" -o -name "glyph_interp" -o -name "checksums.sha256" -o -name "*.json" \) 2>/dev/null || true)

if [ -z "$ARTIFACT_FILES" ]; then
    echo "${YELLOW}⚠ No artifacts found in $ARTIFACTS_DIR${NC}"
    exit 0
fi

echo "${GREEN}Found artifacts to sign:${NC}"
echo "$ARTIFACT_FILES" | while read -r file; do
    echo "  - $file"
done
echo ""

# ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
# 1. Generate SHA256 Checksums
# ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

echo "${BLUE}[1/4] Generating SHA256 checksums...${NC}"

CHECKSUM_FILE="$OUTPUT_DIR/checksums.sha256"

if [ $DRY_RUN -eq 1 ]; then
    echo "${YELLOW}[DRY RUN]${NC} Would generate: $CHECKSUM_FILE"
else
    > "$CHECKSUM_FILE"  # Clear file
    echo "$ARTIFACT_FILES" | while read -r file; do
        if [ -f "$file" ]; then
            sha256sum "$file" >> "$CHECKSUM_FILE"
        fi
    done
    echo "${GREEN}✓ Checksums written to $CHECKSUM_FILE${NC}"
    echo ""
    head -n 5 "$CHECKSUM_FILE"
    echo ""
fi

# ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
# 2. GPG Signing
# ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

if [ $USE_GPG -eq 1 ]; then
    echo "${BLUE}[2/4] GPG signing artifacts...${NC}"

    # Check for GPG
    if ! command -v gpg >/dev/null 2>&1; then
        echo "${RED}✗ GPG not found. Install gpg first.${NC}" >&2
        exit 1
    fi

    # Import GPG key if provided via environment
    if [ -n "$GPG_SIGNING_KEY" ]; then
        echo "${YELLOW}Importing GPG key from environment...${NC}"
        if [ $DRY_RUN -eq 0 ]; then
            echo "$GPG_SIGNING_KEY" | gpg --batch --import 2>/dev/null || {
                echo "${RED}✗ Failed to import GPG key${NC}" >&2
                exit 1
            }
        fi
    fi

    # Sign checksums file
    if [ $DRY_RUN -eq 1 ]; then
        echo "${YELLOW}[DRY RUN]${NC} Would sign: $CHECKSUM_FILE → $CHECKSUM_FILE.asc"
    else
        GPG_OPTS="--batch --yes --armor --detach-sign"
        if [ -n "$GPG_PASSPHRASE" ]; then
            GPG_OPTS="$GPG_OPTS --passphrase '$GPG_PASSPHRASE'"
        fi

        gpg $GPG_OPTS --output "$CHECKSUM_FILE.asc" "$CHECKSUM_FILE" || {
            echo "${RED}✗ GPG signing failed${NC}" >&2
            exit 1
        }
        echo "${GREEN}✓ GPG signature: $CHECKSUM_FILE.asc${NC}"
    fi

    # Sign individual artifacts
    echo "$ARTIFACT_FILES" | while read -r file; do
        if [ -f "$file" ] && [ "$(basename "$file")" != "checksums.sha256" ]; then
            SIGNATURE_FILE="$file.asc"
            if [ $DRY_RUN -eq 1 ]; then
                echo "${YELLOW}[DRY RUN]${NC} Would sign: $file → $SIGNATURE_FILE"
            else
                gpg $GPG_OPTS --output "$SIGNATURE_FILE" "$file" 2>/dev/null || {
                    echo "${YELLOW}⚠ Failed to sign $file${NC}"
                }
            fi
        fi
    done

    echo ""
else
    echo "${BLUE}[2/4] GPG signing skipped (use --gpg to enable)${NC}"
    echo ""
fi

# ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
# 3. Cosign Keyless Signing
# ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

if [ $USE_COSIGN -eq 1 ]; then
    echo "${BLUE}[3/4] Cosign keyless signing...${NC}"

    # Check for Cosign
    if ! command -v cosign >/dev/null 2>&1; then
        echo "${RED}✗ Cosign not found. Install from https://github.com/sigstore/cosign${NC}" >&2
        exit 1
    fi

    # Set experimental mode for keyless signing
    export COSIGN_EXPERIMENTAL=1

    echo "$ARTIFACT_FILES" | while read -r file; do
        if [ -f "$file" ]; then
            SIGNATURE_FILE="$file.sig"
            if [ $DRY_RUN -eq 1 ]; then
                echo "${YELLOW}[DRY RUN]${NC} Would cosign: $file → $SIGNATURE_FILE"
            else
                # Cosign requires OCI registry for keyless mode
                # For local files, generate signature bundle
                cosign sign-blob --bundle "$SIGNATURE_FILE" "$file" 2>/dev/null || {
                    echo "${YELLOW}⚠ Cosign signing failed for $file (may require OIDC login)${NC}"
                }
            fi
        fi
    done

    echo ""
else
    echo "${BLUE}[3/4] Cosign signing skipped (use --cosign to enable)${NC}"
    echo ""
fi

# ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
# 4. Cloud KMS Signing (AWS/GCP/Azure)
# ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

if [ -n "$USE_KMS" ]; then
    echo "${BLUE}[4/4] Cloud KMS signing ($USE_KMS)...${NC}"

    case "$USE_KMS" in
        aws)
            if ! command -v aws >/dev/null 2>&1; then
                echo "${RED}✗ AWS CLI not found${NC}" >&2
                exit 1
            fi

            if [ -z "$AWS_KMS_KEY_ID" ]; then
                echo "${RED}✗ AWS_KMS_KEY_ID not set${NC}" >&2
                exit 1
            fi

            if [ $DRY_RUN -eq 1 ]; then
                echo "${YELLOW}[DRY RUN]${NC} Would use AWS KMS key: $AWS_KMS_KEY_ID"
            else
                # Sign checksums with AWS KMS
                aws kms sign \
                    --key-id "$AWS_KMS_KEY_ID" \
                    --message "file://$CHECKSUM_FILE" \
                    --message-type RAW \
                    --signing-algorithm RSASSA_PSS_SHA_256 \
                    --output text \
                    --query Signature > "$CHECKSUM_FILE.kms"

                echo "${GREEN}✓ AWS KMS signature: $CHECKSUM_FILE.kms${NC}"
            fi
            ;;

        gcp)
            if ! command -v gcloud >/dev/null 2>&1; then
                echo "${RED}✗ gcloud CLI not found${NC}" >&2
                exit 1
            fi

            echo "${YELLOW}⚠ GCP KMS signing not yet implemented${NC}"
            ;;

        azure)
            if ! command -v az >/dev/null 2>&1; then
                echo "${RED}✗ Azure CLI not found${NC}" >&2
                exit 1
            fi

            echo "${YELLOW}⚠ Azure KMS signing not yet implemented${NC}"
            ;;

        *)
            echo "${RED}✗ Unknown KMS provider: $USE_KMS${NC}" >&2
            exit 1
            ;;
    esac

    echo ""
else
    echo "${BLUE}[4/4] Cloud KMS signing skipped${NC}"
    echo ""
fi

# ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
# Summary
# ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

echo "${BLUE}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"
echo "${GREEN}✓ Signing complete${NC}"
echo "${BLUE}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"
echo ""
echo "Generated signatures in: $OUTPUT_DIR"
echo ""

if [ $DRY_RUN -eq 0 ]; then
    find "$OUTPUT_DIR" -type f \( -name "*.asc" -o -name "*.sig" -o -name "*.kms" -o -name "checksums.sha256" \) -ls
fi

echo ""
echo "${BLUE}Verification commands:${NC}"
echo "  GPG:    gpg --verify $CHECKSUM_FILE.asc $CHECKSUM_FILE"
echo "  Cosign: cosign verify-blob --bundle <file>.sig <file>"
echo "  Manual: sha256sum -c $CHECKSUM_FILE"
echo ""
