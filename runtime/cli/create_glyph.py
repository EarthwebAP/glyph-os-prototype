#!/usr/bin/env python3
"""
create_glyph.py - CLI tool to create a glyph with SHA256 content addressing
"""

import argparse
import hashlib
import json
import os
import sys
import tempfile
from pathlib import Path


def get_persistence_path():
    """
    Get the persistence directory path with priority:
    1. /mnt/persistence (NVMe mount)
    2. ./persistence (fallback)
    """
    # Check for NVMe mount
    nvme_path = Path("/mnt/persistence")
    if nvme_path.exists() or os.environ.get("GLYPH_FORCE_NVME"):
        return nvme_path

    # Fallback to local persistence
    script_dir = Path(__file__).parent
    persistence_dir = script_dir / ".." / ".." / "persistence"
    return persistence_dir.resolve()


def create_glyph(content, metadata=None):
    """
    Create a glyph with SHA256 content addressing

    Args:
        content: The content of the glyph
        metadata: Optional metadata dictionary

    Returns:
        tuple: (glyph_id, glyph_dict)
    """
    # Create glyph structure
    glyph = {
        "content": content,
        "metadata": metadata or {}
    }

    # Generate SHA256 hash of content for ID
    content_hash = hashlib.sha256(content.encode('utf-8')).hexdigest()
    glyph["id"] = content_hash

    return content_hash, glyph


def save_glyph(glyph_id, glyph_data):
    """
    Save glyph to persistence directory with atomic write and Merkle-style organization

    Uses content-addressed directory structure:
    persistence/ab/cd/glyph_abcd...json

    Args:
        glyph_id: The SHA256 hash ID
        glyph_data: The glyph dictionary

    Returns:
        str: Path to saved file
    """
    persistence_dir = get_persistence_path()

    # Merkle-style directory organization: first 2 chars, then next 2 chars
    # This prevents directory size issues and improves filesystem performance
    prefix1 = glyph_id[:2]
    prefix2 = glyph_id[2:4]

    target_dir = persistence_dir / prefix1 / prefix2
    target_dir.mkdir(parents=True, exist_ok=True)

    file_path = target_dir / f"glyph_{glyph_id}.json"

    # Atomic write: write to temp file, then rename
    # This ensures no partial writes if process is interrupted
    temp_fd, temp_path = tempfile.mkstemp(
        dir=target_dir,
        prefix=f".tmp_glyph_{glyph_id}_",
        suffix=".json"
    )

    try:
        with os.fdopen(temp_fd, 'w') as f:
            json.dump(glyph_data, f, indent=2)
            f.flush()
            os.fsync(f.fileno())  # Ensure data is written to disk

        # Atomic rename
        os.rename(temp_path, file_path)
    except Exception:
        # Clean up temp file on error
        try:
            os.unlink(temp_path)
        except OSError:
            pass
        raise

    return str(file_path)


def main():
    parser = argparse.ArgumentParser(description="Create a glyph with SHA256 content addressing")
    parser.add_argument("content", help="Content of the glyph")
    parser.add_argument("--metadata", help="Optional metadata as JSON string", default=None)

    args = parser.parse_args()

    # Parse metadata if provided
    metadata = None
    if args.metadata:
        try:
            metadata = json.loads(args.metadata)
        except json.JSONDecodeError:
            print(f"Error: Invalid JSON metadata: {args.metadata}", file=sys.stderr)
            sys.exit(1)

    # Create glyph
    glyph_id, glyph_data = create_glyph(args.content, metadata)

    # Save glyph
    file_path = save_glyph(glyph_id, glyph_data)

    # Output glyph ID
    print(glyph_id)

    return 0


if __name__ == "__main__":
    sys.exit(main())
