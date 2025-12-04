#!/usr/bin/env python3
"""
create_glyph.py - CLI tool to create a glyph with SHA256 content addressing
"""

import argparse
import hashlib
import json
import os
import sys
from pathlib import Path


def get_persistence_path():
    """Get the persistence directory path"""
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
    Save glyph to persistence directory

    Args:
        glyph_id: The SHA256 hash ID
        glyph_data: The glyph dictionary

    Returns:
        str: Path to saved file
    """
    persistence_dir = get_persistence_path()
    persistence_dir.mkdir(parents=True, exist_ok=True)

    file_path = persistence_dir / f"{glyph_id}.json"

    with open(file_path, 'w') as f:
        json.dump(glyph_data, f, indent=2)

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

    # Output result
    print(json.dumps({
        "id": glyph_id,
        "path": file_path,
        "glyph": glyph_data
    }, indent=2))

    return 0


if __name__ == "__main__":
    sys.exit(main())
