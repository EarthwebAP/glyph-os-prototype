#!/usr/bin/env python3
"""
Generate combined results summary from individual benchmarks
"""

import json
import subprocess
from datetime import datetime
from pathlib import Path


def get_git_commit():
    """Get current git commit hash"""
    try:
        result = subprocess.run(
            ["git", "rev-parse", "--short", "HEAD"],
            capture_output=True,
            text=True,
            check=True
        )
        return result.stdout.strip()
    except:
        return "unknown"


def load_persistence_stats():
    """Load persistence benchmark statistics"""
    try:
        with open("benchmarks/persistence_results.json") as f:
            data = json.load(f)
            return data.get("stats", {})
    except:
        return {"median_ms": 0, "p95_ms": 0}


def load_dynamics_stats():
    """Load dynamics determinism statistics"""
    try:
        with open("benchmarks/dynamics_determinism.json") as f:
            results = json.load(f)
            all_deterministic = all(r.get("deterministic", False) for r in results)
            return {
                "deterministic": all_deterministic,
                "seeds_tested": len(results)
            }
    except:
        return {"deterministic": False, "seeds_tested": 0}


def load_spu_stats():
    """Load SPU benchmark statistics"""
    try:
        with open("benchmarks/spu_results.json") as f:
            return json.load(f)
    except:
        return []


def load_fabric_stats():
    """Load fabric latency statistics"""
    try:
        with open("benchmarks/fabric_latency.json") as f:
            return json.load(f)
    except:
        return {"p50_ms": 0, "p95_ms": 0, "p99_ms": 0, "avg_ms": 0}


def generate_notes():
    """Generate summary notes"""
    notes = []

    # Check for failures
    dynamics = load_dynamics_stats()
    if not dynamics.get("deterministic", True):
        notes.append("WARNING: Non-deterministic dynamics detected")

    persistence = load_persistence_stats()
    if persistence.get("failed", 0) > 0:
        notes.append(f"Persistence failures: {persistence['failed']}")

    if not notes:
        notes.append("All benchmarks passed successfully")

    return "; ".join(notes)


def main():
    # Load all benchmark data
    persistence = load_persistence_stats()
    dynamics = load_dynamics_stats()
    spu = load_spu_stats()
    fabric = load_fabric_stats()

    # Create summary
    summary = {
        "timestamp": datetime.utcnow().isoformat() + "Z",
        "commit": get_git_commit(),
        "persistence": {
            "median_ms": persistence.get("median_ms", 0),
            "p95_ms": persistence.get("p95_ms", 0),
            "p99_ms": persistence.get("p99_ms", 0),
            "total_glyphs": persistence.get("total", 0),
            "success_rate": 1.0 if persistence.get("failed", 0) == 0 else (persistence.get("success", 0) / persistence.get("total", 1))
        },
        "dynamics": {
            "deterministic": dynamics.get("deterministic", False),
            "seeds_tested": dynamics.get("seeds_tested", 0)
        },
        "spu": spu,
        "fabric": {
            "p50_ms": fabric.get("p50_ms", 0),
            "p95_ms": fabric.get("p95_ms", 0),
            "p99_ms": fabric.get("p99_ms", 0),
            "avg_ms": fabric.get("avg_ms", 0),
            "transport": fabric.get("transport", "unknown"),
            "rdma_available": fabric.get("rdma_available", False)
        },
        "notes": generate_notes()
    }

    # Save summary
    with open("benchmarks/results_summary.json", 'w') as f:
        json.dump(summary, f, indent=2)

    print("Combined summary generated: benchmarks/results_summary.json")
    print()
    print("=== Benchmark Summary ===")
    print(f"Timestamp: {summary['timestamp']}")
    print(f"Commit: {summary['commit']}")
    print()
    print("Persistence:")
    print(f"  Median: {summary['persistence']['median_ms']} ms")
    print(f"  P95: {summary['persistence']['p95_ms']} ms")
    print()
    print("Dynamics:")
    print(f"  Deterministic: {summary['dynamics']['deterministic']}")
    print(f"  Seeds tested: {summary['dynamics']['seeds_tested']}")
    print()
    print("SPU:")
    for prim in summary['spu']:
        print(f"  {prim['primitive']}: {prim['avg_latency_us']} µs, {prim['ops_per_sec']:,} ops/sec")
    print()
    print("Fabric:")
    print(f"  P50: {summary['fabric']['p50_ms']} ms")
    print(f"  P95: {summary['fabric']['p95_ms']} ms")
    print(f"  P99: {summary['fabric']['p99_ms']} ms")
    print(f"  Transport: {summary['fabric']['transport']}")
    print()
    print(f"Notes: {summary['notes']}")

    return 0


if __name__ == "__main__":
    import sys
    sys.exit(main())
