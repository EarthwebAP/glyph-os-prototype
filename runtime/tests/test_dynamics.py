#!/usr/bin/env python3
"""
Property tests for the dynamics engine

Tests the three core rules with property-based testing principles:
1. Activation Threshold
2. Merge Precedence
3. Decay
"""

import unittest
import sys
from pathlib import Path

# Add runtime modules to path
sys.path.insert(0, str(Path(__file__).parent / ".."))

from dynamics.engine import Glyph, DynamicsEngine


class TestDynamicsProperties(unittest.TestCase):
    """Property tests for dynamics engine"""

    def setUp(self):
        """Set up test fixtures"""
        self.engine = DynamicsEngine(activation_threshold=1.0, decay_rate=0.1)

    def test_activation_threshold_property_below_threshold(self):
        """Property: Glyphs with energy < threshold should not activate"""
        for energy in [0.0, 0.5, 0.9, 0.99]:
            glyph = Glyph("test_id", "test content", {"energy": energy})
            _, activated = self.engine.apply_activation_threshold(glyph)
            self.assertFalse(
                activated,
                f"Glyph with energy {energy} should not activate (threshold=1.0)"
            )

    def test_activation_threshold_property_at_or_above_threshold(self):
        """Property: Glyphs with energy >= threshold should activate"""
        for energy in [1.0, 1.5, 2.0, 10.0]:
            glyph = Glyph("test_id", "test content", {"energy": energy})
            initial_count = glyph.activation_count
            _, activated = self.engine.apply_activation_threshold(glyph)
            self.assertTrue(
                activated,
                f"Glyph with energy {energy} should activate (threshold=1.0)"
            )
            self.assertEqual(
                glyph.activation_count,
                initial_count + 1,
                "Activation should increment counter"
            )

    def test_activation_is_deterministic(self):
        """Property: Activation is deterministic (same input = same output)"""
        glyph1 = Glyph("id1", "content", {"energy": 1.5})
        glyph2 = Glyph("id1", "content", {"energy": 1.5})

        _, activated1 = self.engine.apply_activation_threshold(glyph1)
        _, activated2 = self.engine.apply_activation_threshold(glyph2)

        self.assertEqual(activated1, activated2)
        self.assertEqual(glyph1.activation_count, glyph2.activation_count)

    def test_merge_precedence_property_higher_energy_wins(self):
        """Property: In merge, higher energy glyph takes precedence"""
        high_energy = Glyph("id1", "HIGH", {"energy": 2.0})
        low_energy = Glyph("id2", "LOW", {"energy": 0.5})

        # Test both orders
        merged1 = self.engine.apply_merge_precedence(high_energy, low_energy)
        merged2 = self.engine.apply_merge_precedence(low_energy, high_energy)

        # Both should have same result (commutativity of merge)
        self.assertEqual(merged1.id, merged2.id)
        self.assertTrue("HIGH" in merged1.content)
        self.assertTrue("LOW" in merged1.content)

    def test_merge_precedence_property_energy_conservation(self):
        """Property: Merge conserves total energy"""
        glyph1 = Glyph("id1", "content1", {"energy": 1.5})
        glyph2 = Glyph("id2", "content2", {"energy": 2.3})

        total_energy = glyph1.energy + glyph2.energy
        merged = self.engine.apply_merge_precedence(glyph1, glyph2)

        self.assertAlmostEqual(
            merged.energy,
            total_energy,
            places=10,
            msg="Merge should conserve total energy"
        )

    def test_merge_precedence_property_deterministic(self):
        """Property: Merge is deterministic"""
        g1_a = Glyph("id1", "A", {"energy": 1.0})
        g2_a = Glyph("id2", "B", {"energy": 2.0})
        g1_b = Glyph("id1", "A", {"energy": 1.0})
        g2_b = Glyph("id2", "B", {"energy": 2.0})

        merged_a = self.engine.apply_merge_precedence(g1_a, g2_a)
        merged_b = self.engine.apply_merge_precedence(g1_b, g2_b)

        self.assertEqual(merged_a.id, merged_b.id)
        self.assertEqual(merged_a.content, merged_b.content)
        self.assertEqual(merged_a.energy, merged_b.energy)

    def test_decay_property_energy_decreases(self):
        """Property: Decay always decreases or maintains energy (never increases)"""
        glyph = Glyph("id", "content", {"energy": 10.0})
        initial_energy = glyph.energy

        for time_delta in [1, 5, 10]:
            test_glyph = Glyph("id", "content", {"energy": initial_energy})
            decayed = self.engine.apply_decay(test_glyph, time_delta)
            self.assertLessEqual(
                decayed.energy,
                initial_energy,
                f"Decay should not increase energy (time_delta={time_delta})"
            )

    def test_decay_property_monotonic_decrease(self):
        """Property: Decay is monotonically decreasing over time"""
        glyph = Glyph("id", "content", {"energy": 10.0})

        energies = []
        for t in range(10):
            glyph = self.engine.apply_decay(glyph, 1)
            energies.append(glyph.energy)

        # Check that each energy is less than or equal to previous
        for i in range(1, len(energies)):
            self.assertLessEqual(
                energies[i],
                energies[i-1],
                f"Energy should decrease monotonically: {energies}"
            )

    def test_decay_property_deterministic(self):
        """Property: Decay is deterministic for same time_delta"""
        g1 = Glyph("id", "content", {"energy": 5.0, "last_update_time": 0})
        g2 = Glyph("id", "content", {"energy": 5.0, "last_update_time": 0})

        d1 = self.engine.apply_decay(g1, 5)
        d2 = self.engine.apply_decay(g2, 5)

        self.assertAlmostEqual(d1.energy, d2.energy, places=10)
        self.assertEqual(d1.last_update_time, d2.last_update_time)

    def test_decay_property_zero_time_is_identity(self):
        """Property: Decay with time_delta=0 is identity function"""
        glyph = Glyph("id", "content", {"energy": 5.0, "last_update_time": 10})
        initial_energy = glyph.energy

        decayed = self.engine.apply_decay(glyph, 0)

        self.assertEqual(decayed.energy, initial_energy)

    def test_step_property_combines_rules_deterministically(self):
        """Property: step() combines rules deterministically"""
        g1 = Glyph("id", "content", {"energy": 5.0, "last_update_time": 0})
        g2 = Glyph("id", "content", {"energy": 5.0, "last_update_time": 0})

        result1, info1 = self.engine.step(g1, time_delta=3)
        result2, info2 = self.engine.step(g2, time_delta=3)

        self.assertAlmostEqual(result1.energy, result2.energy, places=10)
        self.assertEqual(result1.activation_count, result2.activation_count)
        self.assertEqual(info1, info2)

    def test_step_property_activates_high_energy_after_threshold(self):
        """Property: step() should activate glyphs with sufficient energy"""
        # High energy glyph that won't decay below threshold
        glyph = Glyph("id", "content", {"energy": 10.0})

        result, info = self.engine.step(glyph, time_delta=1)

        # After decay from 10.0 with rate 0.1: 10.0 * 0.9 = 9.0 (still > 1.0)
        self.assertTrue(info["activated"], "High energy glyph should activate after decay")
        self.assertGreater(info["energy_after_decay"], self.engine.activation_threshold)

    def test_glyph_serialization_roundtrip(self):
        """Property: Glyph serialization is reversible"""
        original = Glyph("test_id", "test content", {
            "energy": 5.5,
            "activation_count": 3,
            "last_update_time": 100,
            "custom": "value"
        })

        # Serialize and deserialize
        data = original.to_dict()
        restored = Glyph.from_dict(data)

        self.assertEqual(original.id, restored.id)
        self.assertEqual(original.content, restored.content)
        self.assertEqual(original.energy, restored.energy)
        self.assertEqual(original.activation_count, restored.activation_count)
        self.assertEqual(original.last_update_time, restored.last_update_time)


if __name__ == "__main__":
    unittest.main()
