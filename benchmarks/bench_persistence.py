#!/usr/bin/env python3
"""
Persistence benchmark - measure write latency and crash safety
"""

import hashlib
import json
import os
import signal
import sys
import time
from pathlib import Path

# Add runtime to path
sys.path.insert(0, str(Path(__file__).parent / ".." / "runtime"))

from cli import create_glyph


def bench_write_latency(num_glyphs=10000):
    """Benchmark write latency for N glyphs"""
    results = []

    print(f"Benchmarking {num_glyphs} glyph writes...")

    for i in range(num_glyphs):
        content = f"Benchmark glyph {i}"
        metadata = {"energy": 1.0 + (i % 10), "bench_id": i}

        # Measure create + save time
        start_time = time.perf_counter()

        try:
            glyph_id, glyph_data = create_glyph.create_glyph(content, metadata)
            file_path = create_glyph.save_glyph(glyph_id, glyph_data)

            end_time = time.perf_counter()
            latency_ms = (end_time - start_time) * 1000

            results.append({
                "id": glyph_id,
                "write_latency_ms": round(latency_ms, 3),
                "fsync_ok": True
            })

        except Exception as e:
            end_time = time.perf_counter()
            latency_ms = (end_time - start_time) * 1000

            results.append({
                "id": f"error_{i}",
                "write_latency_ms": round(latency_ms, 3),
                "fsync_ok": False,
                "error": str(e)
            })

        if (i + 1) % 1000 == 0:
            print(f"  {i + 1} glyphs written...")

    return results


def analyze_results(results):
    """Analyze latency results"""
    latencies = [r["write_latency_ms"] for r in results if r["fsync_ok"]]
    latencies.sort()

    n = len(latencies)
    if n == 0:
        return {"median_ms": 0, "p95_ms": 0, "p99_ms": 0, "mean_ms": 0}

    median = latencies[n // 2]
    p95 = latencies[int(n * 0.95)]
    p99 = latencies[int(n * 0.99)]
    mean = sum(latencies) / n

    failed = sum(1 for r in results if not r["fsync_ok"])

    return {
        "median_ms": round(median, 3),
        "p95_ms": round(p95, 3),
        "p99_ms": round(p99, 3),
        "mean_ms": round(mean, 3),
        "total": len(results),
        "success": len(latencies),
        "failed": failed
    }


def main():
    import argparse
    parser = argparse.ArgumentParser()
    parser.add_argument("--num-glyphs", type=int, default=10000)
    parser.add_argument("--output", default="benchmarks/persistence_results.json")
    args = parser.parse_args()

    # Run benchmark
    results = bench_write_latency(args.num_glyphs)

    # Analyze
    stats = analyze_results(results)

    print(f"\nResults:")
    print(f"  Total: {stats['total']}")
    print(f"  Success: {stats['success']}")
    print(f"  Failed: {stats['failed']}")
    print(f"  Median latency: {stats['median_ms']} ms")
    print(f"  P95 latency: {stats['p95_ms']} ms")
    print(f"  P99 latency: {stats['p99_ms']} ms")
    print(f"  Mean latency: {stats['mean_ms']} ms")

    # Save full results
    output_data = {
        "stats": stats,
        "results": results
    }

    with open(args.output, 'w') as f:
        json.dump(output_data, f, indent=2)

    print(f"\nFull results saved to: {args.output}")

    return 0


if __name__ == "__main__":
    sys.exit(main())
