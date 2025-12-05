#!/usr/bin/env python3
"""
Relation fabric latency benchmark
"""

import json
import sys
import time
from pathlib import Path


def simulate_message_routing(num_messages=10000):
    """Simulate message routing with loopback"""
    latencies = []

    for i in range(num_messages):
        # Simulate message send/receive
        start_time = time.perf_counter()

        # Loopback simulation: minimal processing
        message = {"id": i, "content": f"message_{i}"}
        _ = json.dumps(message)  # Serialize
        _ = json.loads(json.dumps(message))  # Deserialize (loopback)

        end_time = time.perf_counter()

        latency_ms = (end_time - start_time) * 1000
        latencies.append(latency_ms)

    return latencies


def analyze_latencies(latencies):
    """Calculate latency percentiles"""
    sorted_latencies = sorted(latencies)
    n = len(sorted_latencies)

    p50 = sorted_latencies[int(n * 0.50)]
    p95 = sorted_latencies[int(n * 0.95)]
    p99 = sorted_latencies[int(n * 0.99)]
    avg = sum(sorted_latencies) / n

    return {
        "p50_ms": round(p50, 4),
        "p95_ms": round(p95, 4),
        "p99_ms": round(p99, 4),
        "avg_ms": round(avg, 4),
        "transport": "loopback",
        "rdma_available": False,
        "notes": "Loopback simulation - no network transport"
    }


def main():
    import argparse
    parser = argparse.ArgumentParser()
    parser.add_argument("--num-messages", type=int, default=10000)
    parser.add_argument("--output", default="benchmarks/fabric_latency.json")
    args = parser.parse_args()

    print(f"Benchmarking fabric latency ({args.num_messages} messages)...")

    # Run benchmark
    latencies = simulate_message_routing(args.num_messages)

    # Analyze
    stats = analyze_latencies(latencies)

    print(f"\nResults:")
    print(f"  Transport: {stats['transport']}")
    print(f"  RDMA available: {stats['rdma_available']}")
    print(f"  P50 latency: {stats['p50_ms']} ms")
    print(f"  P95 latency: {stats['p95_ms']} ms")
    print(f"  P99 latency: {stats['p99_ms']} ms")
    print(f"  Avg latency: {stats['avg_ms']} ms")

    # Save results
    with open(args.output, 'w') as f:
        json.dump(stats, f, indent=2)

    print(f"\nResults saved to: {args.output}")

    return 0


if __name__ == "__main__":
    sys.exit(main())
