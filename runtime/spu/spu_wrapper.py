"""
SPU merge Python wrapper using subprocess

Simple wrapper that calls the C++ merge_ref binary for testing.
For production, use the pybind11 bindings (bindings.cpp).
"""

import json
import subprocess
import tempfile
from pathlib import Path
from typing import Dict, Any


class Glyph:
    """Python Glyph class compatible with C++ implementation"""

    def __init__(
        self,
        id: str = "",
        content: str = "",
        energy: float = 0.0,
        activation_count: int = 0,
        last_update_time: int = 0,
    ):
        self.id = id
        self.content = content
        self.energy = energy
        self.activation_count = activation_count
        self.last_update_time = last_update_time
        self.parent1_id = ""
        self.parent2_id = ""

    def to_dict(self) -> Dict[str, Any]:
        return {
            "id": self.id,
            "content": self.content,
            "energy": self.energy,
            "activation_count": self.activation_count,
            "last_update_time": self.last_update_time,
        }

    @classmethod
    def from_dict(cls, data: Dict[str, Any]) -> "Glyph":
        g = cls()
        g.id = data.get("id", "")
        g.content = data.get("content", "")
        g.energy = data.get("energy", 0.0)
        g.activation_count = data.get("activation_count", 0)
        g.last_update_time = data.get("last_update_time", 0)
        g.parent1_id = data.get("parent1_id", "")
        g.parent2_id = data.get("parent2_id", "")
        return g

    def __repr__(self):
        return f"<Glyph id='{self.id[:8]}...' energy={self.energy}>"


def merge_via_python(g1: Glyph, g2: Glyph) -> Glyph:
    """
    Pure Python implementation of merge for comparison.
    Matches the C++ implementation logic.
    """
    import hashlib

    # Determine precedence
    if g1.energy >= g2.energy:
        primary, secondary = g1, g2
    else:
        primary, secondary = g2, g1

    # Merge content
    merged_content = f"{primary.content} + {secondary.content}"

    # Compute ID (simplified hash)
    merged_id = hashlib.sha256(merged_content.encode()).hexdigest()

    # Create result
    result = Glyph()
    result.id = merged_id
    result.content = merged_content
    result.energy = primary.energy + secondary.energy
    result.activation_count = max(primary.activation_count, secondary.activation_count)
    result.last_update_time = max(primary.last_update_time, secondary.last_update_time)
    result.parent1_id = primary.id
    result.parent2_id = secondary.id

    return result


# Alias for easier usage
merge = merge_via_python


def benchmark_merge(iterations: int = 10000) -> Dict[str, Any]:
    """
    Benchmark the merge function
    """
    import time

    g1 = Glyph("id1", "content1", 2.0)
    g2 = Glyph("id2", "content2", 3.0)

    # Warmup
    for _ in range(100):
        merge(g1, g2)

    # Benchmark
    start = time.perf_counter()
    for _ in range(iterations):
        merge(g1, g2)
    end = time.perf_counter()

    total_time_ms = (end - start) * 1000
    avg_latency_ms = total_time_ms / iterations
    ops_per_sec = iterations / (end - start)

    return {
        "primitive": "merge",
        "implementation": "python_wrapper",
        "iterations": iterations,
        "total_time_ms": total_time_ms,
        "avg_latency_ms": avg_latency_ms,
        "avg_latency_us": avg_latency_ms * 1000,
        "ops_per_sec": int(ops_per_sec),
    }


if __name__ == "__main__":
    # Test merge
    print("Testing SPU merge wrapper...")

    g1 = Glyph("id1", "content1", 2.0, 5, 100)
    g2 = Glyph("id2", "content2", 3.0, 3, 200)

    print(f"Glyph 1: {g1}")
    print(f"Glyph 2: {g2}")

    result = merge(g1, g2)

    print(f"Merged: {result}")
    print(f"  Content: {result.content}")
    print(f"  Energy: {result.energy} (expected: 5.0)")
    print(f"  Activation count: {result.activation_count} (expected: 5)")
    print(f"  Last update: {result.last_update_time} (expected: 200)")

    # Benchmark
    print("\nBenchmarking...")
    stats = benchmark_merge(10000)
    print(f"  Avg latency: {stats['avg_latency_us']:.2f} µs")
    print(f"  Throughput: {stats['ops_per_sec']:,} ops/sec")
