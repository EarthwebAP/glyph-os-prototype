#!/usr/bin/env python3
"""
SPU merge comparison benchmark

Compares Python and C++ implementations of the merge primitive.
"""

import json
import sys
import time
from pathlib import Path

# Add runtime to path
sys.path.insert(0, str(Path(__file__).parent / ".." / "runtime"))

try:
    # Try to import pybind11 binding
    import spu_merge

    has_cpp_binding = True
except ImportError:
    has_cpp_binding = False

# Import Python wrapper
sys.path.insert(0, str(Path(__file__).parent / ".." / "runtime" / "spu"))
import spu_wrapper


def benchmark_implementation(name, merge_func, glyph_class, iterations=50000):
    """Benchmark a merge implementation"""
    print(f"Benchmarking {name} ({iterations} iterations)...")

    # Create test glyphs
    g1 = glyph_class()
    g1.id = "id1_0000000000000000000000000000000000000000000000000000000000"
    g1.content = "content1"
    g1.energy = 2.0
    g1.activation_count = 0
    g1.last_update_time = 0

    g2 = glyph_class()
    g2.id = "id2_0000000000000000000000000000000000000000000000000000000000"
    g2.content = "content2"
    g2.energy = 3.0
    g2.activation_count = 0
    g2.last_update_time = 0

    # Warmup
    for _ in range(1000):
        merge_func(g1, g2)

    # Benchmark
    latencies = []
    start_total = time.perf_counter()

    for _ in range(iterations):
        start = time.perf_counter()
        result = merge_func(g1, g2)
        end = time.perf_counter()
        latencies.append((end - start) * 1e6)  # µs

    end_total = time.perf_counter()
    total_time_ms = (end_total - start_total) * 1000

    # Compute stats
    latencies.sort()
    min_lat = latencies[0]
    max_lat = latencies[-1]
    median_lat = latencies[len(latencies) // 2]
    p95_lat = latencies[int(len(latencies) * 0.95)]
    p99_lat = latencies[int(len(latencies) * 0.99)]
    mean_lat = sum(latencies) / len(latencies)

    ops_per_sec = iterations / (end_total - start_total)

    print(f"  Mean latency: {mean_lat:.2f} µs")
    print(f"  Median latency: {median_lat:.2f} µs")
    print(f"  P95: {p95_lat:.2f} µs")
    print(f"  P99: {p99_lat:.2f} µs")
    print(f"  Throughput: {ops_per_sec:,.0f} ops/sec")
    print()

    return {
        "implementation": name,
        "iterations": iterations,
        "latency_us": {
            "min": min_lat,
            "max": max_lat,
            "median": median_lat,
            "mean": mean_lat,
            "p95": p95_lat,
            "p99": p99_lat,
        },
        "throughput": {"ops_per_sec": int(ops_per_sec)},
        "total_time_ms": total_time_ms,
    }


def main():
    import argparse

    parser = argparse.ArgumentParser()
    parser.add_argument("--primitive", default="merge")
    parser.add_argument("--iterations", type=int, default=50000)
    parser.add_argument("--out", default="benchmarks/spu_merge_compare.json")
    args = parser.parse_args()

    print("=== SPU Merge Comparison Benchmark ===\n")

    results = []

    # Benchmark Python implementation
    python_result = benchmark_implementation(
        "python", spu_wrapper.merge, spu_wrapper.Glyph, args.iterations
    )
    results.append(python_result)

    # Benchmark C++ implementation (if available)
    if has_cpp_binding:
        cpp_result = benchmark_implementation(
            "cpp_pybind11", spu_merge.merge, spu_merge.Glyph, args.iterations
        )
        results.append(cpp_result)

        # Compute speedup
        speedup = python_result["latency_us"]["mean"] / cpp_result["latency_us"]["mean"]
        print(f"Speedup (C++ vs Python): {speedup:.2f}x\n")

        cpp_result["speedup_vs_python"] = speedup
    else:
        print("Note: C++ binding not available (pybind11 not installed)")
        print("To build: pip install pybind11 && cd runtime/spu && python3 setup.py build_ext --inplace")
        print()

        # Compare to C++ benchmark results if available
        cpp_bench_file = Path("benchmarks/merge_ref_results.json")
        if cpp_bench_file.exists():
            with open(cpp_bench_file) as f:
                cpp_bench = json.load(f)

            cpp_latency = cpp_bench["latency_us"]["mean"]
            speedup = python_result["latency_us"]["mean"] / cpp_latency

            print(f"Comparison to C++ standalone benchmark:")
            print(f"  Python: {python_result['latency_us']['mean']:.2f} µs")
            print(f"  C++ (standalone): {cpp_latency:.2f} µs")
            print(f"  Speedup: {speedup:.2f}x")
            print()

            results.append(
                {
                    "implementation": "cpp_standalone",
                    "latency_us": {"mean": cpp_latency},
                    "throughput": {
                        "ops_per_sec": cpp_bench["throughput"]["ops_per_sec"]
                    },
                    "speedup_vs_python": speedup,
                    "note": "From merge_ref_results.json",
                }
            )

    # Save results
    output = {
        "primitive": args.primitive,
        "iterations": args.iterations,
        "results": results,
    }

    with open(args.out, "w") as f:
        json.dump(output, f, indent=2)

    print(f"Results saved to: {args.out}")

    return 0


if __name__ == "__main__":
    sys.exit(main())
