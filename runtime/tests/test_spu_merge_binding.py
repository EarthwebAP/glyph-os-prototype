#!/usr/bin/env python3
"""
Test SPU merge binding - validates Python wrapper against reference implementation
"""

import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).parent / ".." / "spu"))

import spu_wrapper


def test_merge_energy_precedence():
    """Test that higher energy glyph takes precedence"""
    g1 = spu_wrapper.Glyph("id1", "content1", 2.0)
    g2 = spu_wrapper.Glyph("id2", "content2", 3.0)

    result = spu_wrapper.merge(g1, g2)

    # Higher energy (g2) should be primary
    assert "content2" in result.content
    assert "content1" in result.content
    assert result.content.index("content2") < result.content.index("content1")
    print("✓ Energy precedence test passed")


def test_merge_energy_conservation():
    """Test that energy is conserved (summed)"""
    g1 = spu_wrapper.Glyph("id1", "content1", 2.5)
    g2 = spu_wrapper.Glyph("id2", "content2", 3.5)

    result = spu_wrapper.merge(g1, g2)

    assert abs(result.energy - 6.0) < 1e-9  # Float comparison
    print("✓ Energy conservation test passed")


def test_merge_metadata():
    """Test that metadata is merged correctly (max)"""
    g1 = spu_wrapper.Glyph("id1", "content1", 2.0, activation_count=5, last_update_time=100)
    g2 = spu_wrapper.Glyph("id2", "content2", 3.0, activation_count=3, last_update_time=200)

    result = spu_wrapper.merge(g1, g2)

    assert result.activation_count == 5  # max(5, 3)
    assert result.last_update_time == 200  # max(100, 200)
    print("✓ Metadata merge test passed")


def test_merge_deterministic():
    """Test that merge is deterministic (same inputs = same output)"""
    g1 = spu_wrapper.Glyph("id1", "content1", 2.0)
    g2 = spu_wrapper.Glyph("id2", "content2", 3.0)

    result1 = spu_wrapper.merge(g1, g2)
    result2 = spu_wrapper.merge(g1, g2)

    assert result1.id == result2.id
    assert result1.content == result2.content
    assert result1.energy == result2.energy
    print("✓ Determinism test passed")


def test_merge_provenance():
    """Test that parent IDs are tracked"""
    g1 = spu_wrapper.Glyph("id1_parent", "content1", 2.0)
    g2 = spu_wrapper.Glyph("id2_parent", "content2", 3.0)

    result = spu_wrapper.merge(g1, g2)

    # Check that parent IDs are set
    assert result.parent1_id != ""
    assert result.parent2_id != ""
    print("✓ Provenance test passed")


def test_merge_identical_energy():
    """Test merge when energies are equal"""
    g1 = spu_wrapper.Glyph("id1", "content1", 3.0)
    g2 = spu_wrapper.Glyph("id2", "content2", 3.0)

    result = spu_wrapper.merge(g1, g2)

    # First argument should take precedence when equal
    assert result.energy == 6.0
    assert "content1" in result.content
    assert "content2" in result.content
    print("✓ Equal energy test passed")


def run_all_tests():
    """Run all tests"""
    print("Running SPU merge binding tests...\n")

    tests = [
        test_merge_energy_precedence,
        test_merge_energy_conservation,
        test_merge_metadata,
        test_merge_deterministic,
        test_merge_provenance,
        test_merge_identical_energy,
    ]

    passed = 0
    failed = 0

    for test in tests:
        try:
            test()
            passed += 1
        except AssertionError as e:
            print(f"✗ {test.__name__} failed: {e}")
            failed += 1
        except Exception as e:
            print(f"✗ {test.__name__} error: {e}")
            failed += 1

    print(f"\n{passed}/{len(tests)} tests passed")

    if failed > 0:
        print(f"{failed} tests failed")
        return 1

    print("All tests passed!")
    return 0


if __name__ == "__main__":
    sys.exit(run_all_tests())
