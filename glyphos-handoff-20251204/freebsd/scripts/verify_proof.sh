#!/bin/sh
#
# GlyphOS Proof Verification Script
# Version: 1.0.0
#
# Usage: verify_proof.sh <proof_file.json> [public_key.pem]
#
# This script verifies cryptographic proofs embedded in GDF files or
# benchmark results using OpenSSL and jq.
#
# Proof Format (JSON):
# {
#   "proof_version": "1.0",
#   "proof_type": "benchmark|glyph|substrate",
#   "timestamp": "2025-12-05T15:00:00Z",
#   "payload": { ... },
#   "signature": "base64_encoded_signature",
#   "public_key_id": "key_fingerprint"
# }

set -e

# Configuration
PUBKEY_DIR="${PUBKEY_DIR:-/usr/local/etc/glyphos/keys}"
DEFAULT_PUBKEY="${PUBKEY_DIR}/glyphos_release.pub.pem"

# Colors for output
if [ -t 1 ]; then
    GREEN='\033[0;32m'
    RED='\033[0;31m'
    YELLOW='\033[1;33m'
    NC='\033[0m'
else
    GREEN=''; RED=''; YELLOW=''; NC=''
fi

# Usage
usage() {
    cat << EOF
Usage: $(basename "$0") <proof_file.json> [public_key.pem]

Verify cryptographic proof in JSON file.

Arguments:
  proof_file.json    Path to JSON file containing proof
  public_key.pem     Optional: path to public key (default: ${DEFAULT_PUBKEY})

Environment:
  PUBKEY_DIR         Directory containing public keys (default: /usr/local/etc/glyphos/keys)

Examples:
  $(basename "$0") benchmarks/dma_roundtrip.json
  $(basename "$0") vault/glyph_001.gdf.proof glyphos_dev.pub.pem
  PUBKEY_DIR=/tmp/keys $(basename "$0") test.json

Exit codes:
  0 - Proof verified successfully
  1 - Invalid arguments or missing file
  2 - Dependencies missing (openssl, jq)
  3 - Signature verification failed
  4 - Proof format invalid
EOF
    exit 1
}

# Check dependencies
check_deps() {
    local missing=0

    if ! command -v openssl >/dev/null 2>&1; then
        echo "${RED}Error: openssl not found${NC}" >&2
        echo "Install: pkg install openssl (FreeBSD) or apt-get install openssl (Linux)" >&2
        missing=1
    fi

    if ! command -v jq >/dev/null 2>&1; then
        echo "${RED}Error: jq not found${NC}" >&2
        echo "Install: pkg install jq (FreeBSD) or apt-get install jq (Linux)" >&2
        missing=1
    fi

    if [ $missing -eq 1 ]; then
        exit 2
    fi
}

# Validate proof format
validate_proof_format() {
    local proof_file="$1"

    # Check required fields
    if ! jq -e '.proof_version' "$proof_file" >/dev/null 2>&1; then
        echo "${RED}Error: Missing 'proof_version' field${NC}" >&2
        return 1
    fi

    if ! jq -e '.proof_type' "$proof_file" >/dev/null 2>&1; then
        echo "${RED}Error: Missing 'proof_type' field${NC}" >&2
        return 1
    fi

    if ! jq -e '.payload' "$proof_file" >/dev/null 2>&1; then
        echo "${RED}Error: Missing 'payload' field${NC}" >&2
        return 1
    fi

    if ! jq -e '.signature' "$proof_file" >/dev/null 2>&1; then
        echo "${RED}Error: Missing 'signature' field${NC}" >&2
        return 1
    fi

    return 0
}

# Extract and verify signature
verify_signature() {
    local proof_file="$1"
    local pubkey="$2"

    # Extract payload (everything except signature)
    jq 'del(.signature)' "$proof_file" > /tmp/proof_payload.json

    # Extract signature (base64 encoded)
    jq -r '.signature' "$proof_file" | base64 -d > /tmp/proof_sig.bin 2>/dev/null || {
        echo "${RED}Error: Failed to decode signature${NC}" >&2
        return 1
    }

    # Verify signature
    if openssl dgst -sha256 -verify "$pubkey" -signature /tmp/proof_sig.bin /tmp/proof_payload.json >/dev/null 2>&1; then
        return 0
    else
        return 1
    fi
}

# Main verification
verify_proof() {
    local proof_file="$1"
    local pubkey="$2"

    echo "=================================================="
    echo "  GlyphOS Proof Verification"
    echo "=================================================="
    echo

    # Check file exists
    if [ ! -f "$proof_file" ]; then
        echo "${RED}Error: Proof file not found: ${proof_file}${NC}" >&2
        exit 1
    fi

    # Check public key exists
    if [ ! -f "$pubkey" ]; then
        echo "${RED}Error: Public key not found: ${pubkey}${NC}" >&2
        echo "${YELLOW}Hint: Specify public key as second argument or set PUBKEY_DIR${NC}" >&2
        exit 1
    fi

    echo "Proof file:    ${proof_file}"
    echo "Public key:    ${pubkey}"
    echo

    # Validate format
    echo "[1/4] Validating proof format..."
    if validate_proof_format "$proof_file"; then
        echo "${GREEN}  ✓ Proof format valid${NC}"
    else
        echo "${RED}  ✗ Proof format invalid${NC}"
        exit 4
    fi

    # Extract metadata
    echo
    echo "[2/4] Extracting proof metadata..."
    PROOF_VERSION=$(jq -r '.proof_version' "$proof_file")
    PROOF_TYPE=$(jq -r '.proof_type' "$proof_file")
    TIMESTAMP=$(jq -r '.timestamp // "unknown"' "$proof_file")
    KEY_ID=$(jq -r '.public_key_id // "unknown"' "$proof_file")

    echo "  Version:       ${PROOF_VERSION}"
    echo "  Type:          ${PROOF_TYPE}"
    echo "  Timestamp:     ${TIMESTAMP}"
    echo "  Key ID:        ${KEY_ID}"

    # Verify signature
    echo
    echo "[3/4] Verifying cryptographic signature..."
    if verify_signature "$proof_file" "$pubkey"; then
        echo "${GREEN}  ✓ Signature valid${NC}"
    else
        echo "${RED}  ✗ Signature verification FAILED${NC}"
        rm -f /tmp/proof_payload.json /tmp/proof_sig.bin
        exit 3
    fi

    # Verify payload integrity
    echo
    echo "[4/4] Verifying payload integrity..."
    PAYLOAD_SIZE=$(jq '.payload | length' "$proof_file")
    echo "  Payload size:  ${PAYLOAD_SIZE} fields"

    # Check for required payload fields based on type
    case "$PROOF_TYPE" in
        benchmark)
            if jq -e '.payload.test_name' "$proof_file" >/dev/null 2>&1; then
                TEST_NAME=$(jq -r '.payload.test_name' "$proof_file")
                echo "  Test name:     ${TEST_NAME}"
            fi
            ;;
        glyph)
            if jq -e '.payload.glyph_id' "$proof_file" >/dev/null 2>&1; then
                GLYPH_ID=$(jq -r '.payload.glyph_id' "$proof_file")
                echo "  Glyph ID:      ${GLYPH_ID}"
            fi
            ;;
        substrate)
            if jq -e '.payload.checksum' "$proof_file" >/dev/null 2>&1; then
                CHECKSUM=$(jq -r '.payload.checksum' "$proof_file")
                echo "  Checksum:      ${CHECKSUM}"
            fi
            ;;
    esac

    echo "${GREEN}  ✓ Payload integrity verified${NC}"

    # Cleanup
    rm -f /tmp/proof_payload.json /tmp/proof_sig.bin

    # Success
    echo
    echo "=================================================="
    echo "${GREEN}  ✅ PROOF VERIFIED SUCCESSFULLY${NC}"
    echo "=================================================="
    echo

    return 0
}

# Parse arguments
if [ $# -lt 1 ]; then
    usage
fi

PROOF_FILE="$1"
PUBKEY="${2:-${DEFAULT_PUBKEY}}"

# Check dependencies
check_deps

# Verify proof
verify_proof "$PROOF_FILE" "$PUBKEY"

exit 0
