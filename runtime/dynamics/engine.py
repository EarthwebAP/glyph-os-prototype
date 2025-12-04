#!/usr/bin/env python3
"""
Dynamics Engine - Deterministic rule engine for glyph evolution

Implements three core rules:
1. Activation Threshold - Glyphs activate when energy exceeds threshold
2. Merge Precedence - Higher energy glyphs take precedence in merges
3. Decay - Glyph energy decays over time deterministically
"""

import hashlib
import json
from typing import Dict, List, Optional, Tuple


class Glyph:
    """Glyph with dynamics properties"""

    def __init__(self, id: str, content: str, metadata: Optional[Dict] = None):
        self.id = id
        self.content = content
        self.metadata = metadata or {}
        self.energy = self.metadata.get("energy", 0.0)
        self.activation_count = self.metadata.get("activation_count", 0)
        self.last_update_time = self.metadata.get("last_update_time", 0)

    def to_dict(self) -> Dict:
        """Convert glyph to dictionary"""
        return {
            "id": self.id,
            "content": self.content,
            "metadata": {
                "energy": self.energy,
                "activation_count": self.activation_count,
                "last_update_time": self.last_update_time,
                **{k: v for k, v in self.metadata.items()
                   if k not in ["energy", "activation_count", "last_update_time"]}
            }
        }

    @classmethod
    def from_dict(cls, data: Dict) -> "Glyph":
        """Create glyph from dictionary"""
        return cls(
            id=data["id"],
            content=data["content"],
            metadata=data.get("metadata", {})
        )


class DynamicsEngine:
    """Deterministic rule engine for glyph evolution"""

    def __init__(self, activation_threshold: float = 1.0, decay_rate: float = 0.1):
        """
        Initialize dynamics engine

        Args:
            activation_threshold: Energy threshold for activation
            decay_rate: Rate of energy decay per time unit (0.0-1.0)
        """
        self.activation_threshold = activation_threshold
        self.decay_rate = max(0.0, min(1.0, decay_rate))  # Clamp to [0, 1]

    def apply_activation_threshold(self, glyph: Glyph) -> Tuple[Glyph, bool]:
        """
        Rule 1: Activation Threshold

        A glyph activates when its energy exceeds the threshold.
        Activation increments the activation counter.

        Args:
            glyph: Input glyph

        Returns:
            (updated_glyph, activated)
        """
        activated = False

        if glyph.energy >= self.activation_threshold:
            glyph.activation_count += 1
            activated = True

        return glyph, activated

    def apply_merge_precedence(self, glyph1: Glyph, glyph2: Glyph) -> Glyph:
        """
        Rule 2: Merge Precedence

        When two glyphs merge, the one with higher energy takes precedence.
        Energy is summed, content is concatenated with precedence.

        Args:
            glyph1: First glyph
            glyph2: Second glyph

        Returns:
            Merged glyph
        """
        # Determine precedence
        if glyph1.energy >= glyph2.energy:
            primary, secondary = glyph1, glyph2
        else:
            primary, secondary = glyph2, glyph1

        # Create merged content (primary content takes precedence)
        merged_content = f"{primary.content} + {secondary.content}"

        # Compute new ID from merged content
        merged_id = hashlib.sha256(merged_content.encode('utf-8')).hexdigest()

        # Sum energies
        merged_energy = primary.energy + secondary.energy

        # Create merged glyph
        merged = Glyph(
            id=merged_id,
            content=merged_content,
            metadata={
                "energy": merged_energy,
                "activation_count": max(primary.activation_count, secondary.activation_count),
                "last_update_time": max(primary.last_update_time, secondary.last_update_time),
                "merged_from": [primary.id, secondary.id]
            }
        )

        return merged

    def apply_decay(self, glyph: Glyph, time_delta: int) -> Glyph:
        """
        Rule 3: Decay

        Glyph energy decays exponentially over time.
        Decay is deterministic based on time_delta.

        Args:
            glyph: Input glyph
            time_delta: Time units elapsed since last update

        Returns:
            Glyph with decayed energy
        """
        # Exponential decay: E_new = E_old * (1 - decay_rate)^time_delta
        decay_factor = (1.0 - self.decay_rate) ** time_delta
        glyph.energy = glyph.energy * decay_factor
        glyph.last_update_time += time_delta

        return glyph

    def step(self, glyph: Glyph, time_delta: int = 1) -> Tuple[Glyph, Dict]:
        """
        Execute one dynamics step on a glyph

        Applies rules in order:
        1. Decay (time-based)
        2. Activation threshold check

        Args:
            glyph: Input glyph
            time_delta: Time units since last step

        Returns:
            (updated_glyph, step_info)
        """
        step_info = {
            "initial_energy": glyph.energy,
            "time_delta": time_delta,
            "activated": False
        }

        # Apply decay
        glyph = self.apply_decay(glyph, time_delta)
        step_info["energy_after_decay"] = glyph.energy

        # Check activation
        glyph, activated = self.apply_activation_threshold(glyph)
        step_info["activated"] = activated
        step_info["final_energy"] = glyph.energy
        step_info["activation_count"] = glyph.activation_count

        return glyph, step_info
