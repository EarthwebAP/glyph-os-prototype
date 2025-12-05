#!/usr/bin/env python3
"""
Enhanced persistence benchmark with async batching experiments
"""

import hashlib
import json
import os
import sys
import time
from pathlib import Path
from concurrent.futures import ThreadPoolExecutor

sys.path.insert(0, str(Path(__file__).parent / ".." / "runtime"))
from cli import create_glyph


class PersistenceBenchmark:
    """Persistence benchmark with batching support"""

    def __init__(self, batch_window_ms=0):
        self.batch_window_ms = batch_window_ms
        self.batch = []
        self.last_flush = time.time()

    def bench_write(self, count=10000, parallel=1):
        """Benchmark write latency"""
        results = []

        print(f"Benchmarking {count} writes (parallel={parallel}, batch_window={self.batch_window_ms}ms)...")

        if parallel > 1:
            results = self._bench_parallel(count, parallel)
        else:
            results = self._bench_sequential(count)

        return results

    def _bench_sequential(self, count):
        """Sequential write benchmark"""
        results = []

        for i in range(count):
            content = f"Persistence bench {i}"
            metadata = {"energy": 1.0 + (i % 10), "bench_id": i}

            start_time = time.perf_counter()

            try:
                glyph_id, glyph_data = create_glyph.create_glyph(content, metadata)

                # Apply batching if configured
                if self.batch_window_ms > 0:
                    self.batch.append((glyph_id, glyph_data))

                    # Flush batch if window expired
                    current_time = time.time()
                    if (current_time - self.last_flush) * 1000 >= self.batch_window_ms:
                        self._flush_batch()
                else:
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
                print(f"  {i + 1} writes completed...")

        # Flush any remaining batch
        if self.batch:
            self._flush_batch()

        return results

    def _bench_parallel(self, count, workers):
        """Parallel write benchmark"""
        results = []

        def write_one(i):
            content = f"Persistence bench {i}"
            metadata = {"energy": 1.0 + (i % 10), "bench_id": i}

            start_time = time.perf_counter()

            try:
                glyph_id, glyph_data = create_glyph.create_glyph(content, metadata)
                file_path = create_glyph.save_glyph(glyph_id, glyph_data)

                end_time = time.perf_counter()
                latency_ms = (end_time - start_time) * 1000

                return {
                    "id": glyph_id,
                    "write_latency_ms": round(latency_ms, 3),
                    "fsync_ok": True
                }

            except Exception as e:
                end_time = time.perf_counter()
                latency_ms = (end_time - start_time) * 1000

                return {
                    "id": f"error_{i}",
                    "write_latency_ms": round(latency_ms, 3),
                    "fsync_ok": False,
                    "error": str(e)
                }

        with ThreadPoolExecutor(max_workers=workers) as executor:
            results = list(executor.map(write_one, range(count)))

        return results

    def _flush_batch(self):
        """Flush batched writes"""
        for glyph_id, glyph_data in self.batch:
            create_glyph.save_glyph(glyph_id, glyph_data)

        self.batch = []
        self.last_flush = time.time()


def analyze_results(results):
    """Analyze latency results"""
    latencies = [r["write_latency_ms"] for r in results if r["fsync_ok"]]
    latencies.sort()

    n = len(latencies)
    if n == 0:
        return {
            "median_ms": 0, "p95_ms": 0, "p99_ms": 0, "mean_ms": 0,
            "total": len(results), "success": 0, "failed": len(results)
        }

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
        "min_ms": round(latencies[0], 3),
        "max_ms": round(latencies[-1], 3),
        "total": len(results),
        "success": len(latencies),
        "failed": failed
    }


def main():
    import argparse
    parser = argparse.ArgumentParser()
    parser.add_argument("--count", type=int, default=10000, help="Number of glyphs to create")
    parser.add_argument("--batch-window-ms", type=int, default=0, help="Batch window in ms (0=disabled)")
    parser.add_argument("--parallel", type=int, default=1, help="Parallel workers (1=sequential)")
    parser.add_argument("--out", default="benchmarks/persistence_results.json")
    args = parser.parse_args()

    # Run benchmark
    bench = PersistenceBenchmark(batch_window_ms=args.batch_window_ms)
    results = bench.bench_write(count=args.count, parallel=args.parallel)

    # Analyze
    stats = analyze_results(results)

    print(f"\nResults:")
    print(f"  Total: {stats['total']}")
    print(f"  Success: {stats['success']}")
    print(f"  Failed: {stats['failed']}")
    print(f"  Median: {stats['median_ms']} ms")
    print(f"  P95: {stats['p95_ms']} ms")
    print(f"  P99: {stats['p99_ms']} ms")
    print(f"  Mean: {stats['mean_ms']} ms")
    print(f"  Min: {stats['min_ms']} ms")
    print(f"  Max: {stats['max_ms']} ms")

    # Save results
    output_data = {
        "config": {
            "count": args.count,
            "batch_window_ms": args.batch_window_ms,
            "parallel": args.parallel
        },
        "stats": stats,
        "results": results[:100]  # Save first 100 for inspection
    }

    with open(args.out, 'w') as f:
        json.dump(output_data, f, indent=2)

    print(f"\nResults saved to: {args.out}")

    return 0


if __name__ == "__main__":
    sys.exit(main())
