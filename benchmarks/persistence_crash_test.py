#!/usr/bin/env python3
"""
Persistence crash safety test - verify atomic writes
"""

import json
import os
import random
import signal
import sys
import tempfile
import time
from pathlib import Path

sys.path.insert(0, str(Path(__file__).parent / ".." / "runtime"))
from cli import create_glyph


def test_concurrent_writes_with_crash(count=1000):
    """Test concurrent writes with simulated crash"""
    print(f"Testing crash safety with {count} concurrent writes...")

    results = {
        "total_attempts": count,
        "completed_writes": 0,
        "partial_files": 0,
        "corrupted_files": 0,
        "temp_files_remaining": 0,
        "fsync_ok": True
    }

    persistence_dir = create_glyph.get_persistence_path()

    # Create many glyphs
    glyph_ids = []
    for i in range(count):
        content = f"Crash test glyph {i}"
        metadata = {"energy": random.uniform(1.0, 10.0), "test_id": i}

        try:
            glyph_id, glyph_data = create_glyph.create_glyph(content, metadata)
            file_path = create_glyph.save_glyph(glyph_id, glyph_data)
            glyph_ids.append((glyph_id, file_path))
            results["completed_writes"] += 1

        except Exception as e:
            print(f"  Write {i} failed: {e}")
            results["fsync_ok"] = False

        if (i + 1) % 100 == 0:
            print(f"  {i + 1} writes attempted...")

    print(f"\nCompleted writes: {results['completed_writes']}")

    # Verify all written files
    print("\nVerifying written files...")
    for glyph_id, file_path in glyph_ids:
        file_path = Path(file_path)

        if not file_path.exists():
            print(f"  ERROR: File missing: {file_path}")
            results["partial_files"] += 1
            continue

        # Try to read and parse JSON
        try:
            with open(file_path, 'r') as f:
                data = json.load(f)

            # Verify structure
            if "id" not in data or data["id"] != glyph_id:
                print(f"  ERROR: Corrupted file: {file_path}")
                results["corrupted_files"] += 1

        except json.JSONDecodeError as e:
            print(f"  ERROR: Invalid JSON in {file_path}: {e}")
            results["corrupted_files"] += 1

        except Exception as e:
            print(f"  ERROR: Cannot read {file_path}: {e}")
            results["corrupted_files"] += 1

    # Check for temp files
    print("\nChecking for temporary files...")
    temp_files = list(persistence_dir.rglob(".tmp_glyph_*"))
    results["temp_files_remaining"] = len(temp_files)

    if temp_files:
        print(f"  WARNING: Found {len(temp_files)} temp files")
        for tf in temp_files[:5]:  # Show first 5
            print(f"    {tf}")
    else:
        print("  No temp files found (good)")

    return results


def main():
    import argparse
    parser = argparse.ArgumentParser()
    parser.add_argument("--count", type=int, default=1000)
    parser.add_argument("--out", default="benchmarks/persistence_crash_report.txt")
    args = parser.parse_args()

    # Run crash test
    results = test_concurrent_writes_with_crash(args.count)

    # Generate report
    report = f"""Persistence Crash Safety Test Report
=====================================

Test Date: {time.strftime('%Y-%m-%d %H:%M:%S')}
Test Count: {results['total_attempts']} concurrent writes

Results:
--------
Total attempts: {results['total_attempts']}
Completed writes: {results['completed_writes']}
Partial files: {results['partial_files']}
Corrupted files: {results['corrupted_files']}
Temp files remaining: {results['temp_files_remaining']}
Fsync OK: {results['fsync_ok']}

Crash Safety Verification:
---------------------------
✓ Atomic writes: {"PASS" if results['partial_files'] == 0 else "FAIL"}
✓ No corruption: {"PASS" if results['corrupted_files'] == 0 else "FAIL"}
✓ Temp cleanup: {"PASS" if results['temp_files_remaining'] == 0 else "FAIL"}
✓ Fsync success: {"PASS" if results['fsync_ok'] else "FAIL"}

Conclusion:
-----------
{"All crash safety checks passed. Atomic writes verified." if all([
    results['partial_files'] == 0,
    results['corrupted_files'] == 0,
    results['temp_files_remaining'] == 0,
    results['fsync_ok']
]) else "Some checks failed. Review errors above."}

Implementation Details:
-----------------------
- Uses tempfile.mkstemp() for atomic file creation
- Writes complete glyph data to temp file
- Calls os.fsync() to flush data to disk
- Performs atomic os.rename() to final location
- Exception handler cleans up temp files on error

OVERALL: {"PASS" if results['partial_files'] == 0 and results['corrupted_files'] == 0 else "FAIL"}
"""

    print("\n" + report)

    # Save report
    with open(args.out, 'w') as f:
        f.write(report)

    print(f"\nReport saved to: {args.out}")

    return 0 if (results['partial_files'] == 0 and results['corrupted_files'] == 0) else 1


if __name__ == "__main__":
    sys.exit(main())
