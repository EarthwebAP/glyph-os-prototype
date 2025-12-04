#!/usr/bin/env python3
"""
query_glyph.py - CLI tool to query a glyph by ID
"""

import argparse
import json
import sys
from pathlib import Path


def get_persistence_path():
    """
    Get the persistence directory path with priority:
    1. /mnt/persistence (NVMe mount)
    2. ./persistence (fallback)
    """
    import os
    # Check for NVMe mount
    nvme_path = Path("/mnt/persistence")
    if nvme_path.exists() or os.environ.get("GLYPH_FORCE_NVME"):
        return nvme_path

    # Fallback to local persistence
    script_dir = Path(__file__).parent
    persistence_dir = script_dir / ".." / ".." / "persistence"
    return persistence_dir.resolve()


def query_glyph(glyph_id):
    """
    Query a glyph by ID from persistence directory using Merkle-style paths

    Args:
        glyph_id: The SHA256 hash ID of the glyph

    Returns:
        dict: The glyph data, or None if not found
    """
    persistence_dir = get_persistence_path()

    # Merkle-style directory organization
    prefix1 = glyph_id[:2]
    prefix2 = glyph_id[2:4]

    file_path = persistence_dir / prefix1 / prefix2 / f"glyph_{glyph_id}.json"

    if not file_path.exists():
        return None

    with open(file_path, 'r') as f:
        return json.load(f)


def main():
    parser = argparse.ArgumentParser(description="Query a glyph by ID")
    parser.add_argument("id", help="SHA256 hash ID of the glyph")
    parser.add_argument("--quiet", "-q", action="store_true", help="Only output the glyph data")

    args = parser.parse_args()

    # Query glyph
    glyph_data = query_glyph(args.id)

    if glyph_data is None:
        if not args.quiet:
            print(f"Error: Glyph with ID '{args.id}' not found", file=sys.stderr)
        sys.exit(1)

    # Output result
    print(json.dumps(glyph_data, indent=2))
    return 0


if __name__ == "__main__":
    sys.exit(main())
