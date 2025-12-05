#!/usr/bin/env python3
"""
SPU (Symbolic Processing Unit) microbenchmarks
"""

import hashlib
import json
import sys
import time
from pathlib import Path

sys.path.insert(0, str(Path(__file__).parent / ".." / "runtime"))

from dynamics.engine import Glyph, DynamicsEngine


def benchmark_primitive(name, func, iterations=10000):
    """Benchmark a single primitive operation"""
    print(f"Benchmarking {name} ({iterations} iterations)...")

    # Warmup
    for _ in range(100):
        func()

    # Actual benchmark
    start_time = time.perf_counter()
    for _ in range(iterations):
        func()
    end_time = time.perf_counter()

    total_time_us = (end_time - start_time) * 1_000_000
    avg_latency_us = total_time_us / iterations
    ops_per_sec = iterations / (end_time - start_time)

    return {
        "primitive": name,
        "avg_latency_us": round(avg_latency_us, 2),
        "ops_per_sec": int(ops_per_sec)
    }


def bench_merge():
    """Benchmark glyph merge operation"""
    engine = DynamicsEngine()

    g1 = Glyph("id1", "content1", {"energy": 2.0})
    g2 = Glyph("id2", "content2", {"energy": 3.0})

    def merge_op():
        return engine.apply_merge_precedence(g1, g2)

    return merge_op


def bench_transform():
    """Benchmark glyph transformation (decay)"""
    engine = DynamicsEngine(decay_rate=0.1)
    glyph = Glyph("id", "content", {"energy": 5.0, "last_update_time": 0})

    def transform_op():
        g = Glyph("id", "content", {"energy": 5.0, "last_update_time": 0})
        return engine.apply_decay(g, 1)

    return transform_op


def bench_match():
    """Benchmark glyph matching (activation threshold check)"""
    engine = DynamicsEngine(activation_threshold=1.0)
    glyph = Glyph("id", "content", {"energy": 2.0})

    def match_op():
        g = Glyph("id", "content", {"energy": 2.0})
        return engine.apply_activation_threshold(g)

    return match_op


def bench_resonate():
    """Benchmark glyph resonance calculation (hash computation)"""
    def resonate_op():
        content = "test content for resonance"
        return hashlib.sha256(content.encode()).hexdigest()

    return resonate_op


def bench_prune():
    """Benchmark glyph pruning (serialization)"""
    glyph = Glyph("id", "content", {"energy": 5.0, "activation_count": 10})

    def prune_op():
        return glyph.to_dict()

    return prune_op


def main():
    import argparse
    parser = argparse.ArgumentParser()
    parser.add_argument("--iterations", type=int, default=10000)
    parser.add_argument("--output", default="benchmarks/spu_results.json")
    args = parser.parse_args()

    primitives = [
        ("merge", bench_merge()),
        ("transform", bench_transform()),
        ("match", bench_match()),
        ("resonate", bench_resonate()),
        ("prune", bench_prune())
    ]

    results = []

    for name, func in primitives:
        result = benchmark_primitive(name, func, args.iterations)
        results.append(result)
        print(f"  {name}: {result['avg_latency_us']:.2f} µs/op, {result['ops_per_sec']:,} ops/sec")

    # Find slowest primitive
    slowest = max(results, key=lambda r: r['avg_latency_us'])
    print(f"\nSlowest primitive: {slowest['primitive']} ({slowest['avg_latency_us']:.2f} µs)")

    # Save results
    with open(args.output, 'w') as f:
        json.dump(results, f, indent=2)

    print(f"\nResults saved to: {args.output}")

    # Create placeholder flamegraph
    flamegraph_path = Path(args.output).parent / "spu_flame.svg"
    with open(flamegraph_path, 'w') as f:
        f.write(f"""<?xml version="1.0" standalone="no"?>
<!DOCTYPE svg PUBLIC "-//W3C//DTD SVG 1.1//EN" "http://www.w3.org/Graphics/SVG/1.1/DTD/svg11.dtd">
<svg version="1.1" width="1200" height="400" xmlns="http://www.w3.org/2000/svg">
  <text x="10" y="30" font-family="monospace" font-size="16">SPU Flamegraph - Slowest Primitive: {slowest['primitive']}</text>
  <text x="10" y="60" font-family="monospace" font-size="14">Avg latency: {slowest['avg_latency_us']:.2f} µs</text>
  <text x="10" y="90" font-family="monospace" font-size="14">Ops/sec: {slowest['ops_per_sec']:,}</text>
  <text x="10" y="130" font-family="monospace" font-size="12" fill="#666">
    Note: Full flamegraph requires profiling tools (py-spy, cProfile, etc.)
  </text>
  <rect x="10" y="150" width="800" height="40" fill="#e74c3c" />
  <text x="15" y="175" font-family="monospace" font-size="14" fill="white">{slowest['primitive']} - {slowest['avg_latency_us']:.2f} µs</text>
</svg>""")

    print(f"Placeholder flamegraph saved to: {flamegraph_path}")

    return 0


if __name__ == "__main__":
    sys.exit(main())
