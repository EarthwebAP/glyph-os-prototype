#!/usr/bin/env python3
"""
GlyphOS Proof Verification Script (Python version)
Alternative to verify_proof.sh for environments without jq

Usage: python3 verify_proof.py <proof_file.json> [public_key.pem]
"""

import sys
import json
import base64
import subprocess
import os
from pathlib import Path

# Colors
GREEN = '\033[0;32m'
RED = '\033[0;31m'
YELLOW = '\033[1;33m'
NC = '\033[0m'

PUBKEY_DIR = os.environ.get('PUBKEY_DIR', '/usr/local/etc/glyphos/keys')
DEFAULT_PUBKEY = f"{PUBKEY_DIR}/glyphos_release.pub.pem"


def check_deps():
    """Check for required dependencies"""
    try:
        subprocess.run(['openssl', 'version'], capture_output=True, check=True)
    except (subprocess.CalledProcessError, FileNotFoundError):
        print(f"{RED}Error: openssl not found{NC}", file=sys.stderr)
        sys.exit(2)


def validate_proof_format(proof_data):
    """Validate proof JSON structure"""
    required_fields = ['proof_version', 'proof_type', 'payload', 'signature']

    for field in required_fields:
        if field not in proof_data:
            print(f"{RED}Error: Missing '{field}' field{NC}", file=sys.stderr)
            return False

    return True


def verify_signature(proof_file, pubkey):
    """Verify RSA signature using OpenSSL"""
    try:
        with open(proof_file, 'r') as f:
            proof_data = json.load(f)

        # Extract payload (everything except signature)
        payload = {k: v for k, v in proof_data.items() if k != 'signature'}

        # Write payload to temp file
        with open('/tmp/proof_payload.json', 'w') as f:
            json.dump(payload, f, indent=2, sort_keys=True)

        # Decode signature
        signature_b64 = proof_data['signature']
        signature_bin = base64.b64decode(signature_b64)

        with open('/tmp/proof_sig.bin', 'wb') as f:
            f.write(signature_bin)

        # Verify with OpenSSL
        result = subprocess.run(
            ['openssl', 'dgst', '-sha256', '-verify', pubkey,
             '-signature', '/tmp/proof_sig.bin', '/tmp/proof_payload.json'],
            capture_output=True
        )

        # Cleanup
        os.remove('/tmp/proof_payload.json')
        os.remove('/tmp/proof_sig.bin')

        return result.returncode == 0

    except Exception as e:
        print(f"{RED}Error during verification: {e}{NC}", file=sys.stderr)
        return False


def verify_proof(proof_file, pubkey):
    """Main verification function"""
    print("=" * 50)
    print("  GlyphOS Proof Verification (Python)")
    print("=" * 50)
    print()

    # Check files exist
    if not os.path.exists(proof_file):
        print(f"{RED}Error: Proof file not found: {proof_file}{NC}", file=sys.stderr)
        sys.exit(1)

    if not os.path.exists(pubkey):
        print(f"{RED}Error: Public key not found: {pubkey}{NC}", file=sys.stderr)
        print(f"{YELLOW}Hint: Specify public key as second argument or set PUBKEY_DIR{NC}", file=sys.stderr)
        sys.exit(1)

    print(f"Proof file:    {proof_file}")
    print(f"Public key:    {pubkey}")
    print()

    # Load proof
    try:
        with open(proof_file, 'r') as f:
            proof_data = json.load(f)
    except json.JSONDecodeError as e:
        print(f"{RED}Error: Invalid JSON: {e}{NC}", file=sys.stderr)
        sys.exit(4)

    # Validate format
    print("[1/4] Validating proof format...")
    if validate_proof_format(proof_data):
        print(f"{GREEN}  ✓ Proof format valid{NC}")
    else:
        print(f"{RED}  ✗ Proof format invalid{NC}")
        sys.exit(4)

    # Extract metadata
    print()
    print("[2/4] Extracting proof metadata...")
    proof_version = proof_data.get('proof_version', 'unknown')
    proof_type = proof_data.get('proof_type', 'unknown')
    timestamp = proof_data.get('timestamp', 'unknown')
    key_id = proof_data.get('public_key_id', 'unknown')

    print(f"  Version:       {proof_version}")
    print(f"  Type:          {proof_type}")
    print(f"  Timestamp:     {timestamp}")
    print(f"  Key ID:        {key_id}")

    # Verify signature
    print()
    print("[3/4] Verifying cryptographic signature...")
    if verify_signature(proof_file, pubkey):
        print(f"{GREEN}  ✓ Signature valid{NC}")
    else:
        print(f"{RED}  ✗ Signature verification FAILED{NC}")
        sys.exit(3)

    # Verify payload
    print()
    print("[4/4] Verifying payload integrity...")
    payload = proof_data.get('payload', {})
    print(f"  Payload size:  {len(payload)} fields")

    # Type-specific checks
    if proof_type == 'benchmark' and 'test_name' in payload:
        print(f"  Test name:     {payload['test_name']}")
    elif proof_type == 'glyph' and 'glyph_id' in payload:
        print(f"  Glyph ID:      {payload['glyph_id']}")
    elif proof_type == 'substrate' and 'checksum' in payload:
        print(f"  Checksum:      {payload['checksum']}")

    print(f"{GREEN}  ✓ Payload integrity verified{NC}")

    # Success
    print()
    print("=" * 50)
    print(f"{GREEN}  ✅ PROOF VERIFIED SUCCESSFULLY{NC}")
    print("=" * 50)
    print()

    return 0


def main():
    if len(sys.argv) < 2:
        print(f"Usage: {sys.argv[0]} <proof_file.json> [public_key.pem]", file=sys.stderr)
        sys.exit(1)

    proof_file = sys.argv[1]
    pubkey = sys.argv[2] if len(sys.argv) > 2 else DEFAULT_PUBKEY

    check_deps()
    sys.exit(verify_proof(proof_file, pubkey))


if __name__ == '__main__':
    main()
