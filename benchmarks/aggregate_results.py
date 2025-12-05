#!/usr/bin/env python3
"""
Aggregate all benchmark results into a single summary JSON
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


def load_json(path):
    """Load JSON file safely"""
    try:
        with open(path) as f:
            return json.load(f)
    except:
        return None


def aggregate_persistence():
    """Aggregate persistence benchmark results"""
    baseline = load_json("benchmarks/persistence_results.json")
    batched = load_json("benchmarks/persistence_results_batch5.json")

    if not baseline:
        return {"error": "baseline results not found"}

    result = {
        "baseline": baseline.get("stats", {}),
        "config": baseline.get("config", {})
    }

    if batched:
        result["batched"] = {
            "stats": batched.get("stats", {}),
            "config": batched.get("config", {}),
            "improvement": {
                "median_reduction_pct": round((1 - batched["stats"]["median_ms"] / baseline["stats"]["median_ms"]) * 100, 1) if baseline["stats"]["median_ms"] > 0 else 0,
                "p99_reduction_pct": round((1 - batched["stats"]["p99_ms"] / baseline["stats"]["p99_ms"]) * 100, 1) if baseline["stats"]["p99_ms"] > 0 else 0
            }
        }

    return result


def aggregate_dynamics():
    """Aggregate dynamics determinism results"""
    data = load_json("benchmarks/dynamics_determinism.json")

    if not data:
        return {"error": "dynamics results not found"}

    all_deterministic = all(r.get("deterministic", False) for r in data)

    return {
        "deterministic": all_deterministic,
        "seeds_tested": len(data),
        "failing_seeds": [r["seed"] for r in data if not r.get("deterministic", False)]
    }


def aggregate_spu():
    """Aggregate SPU microbenchmark results"""
    data = load_json("benchmarks/spu_results.json")

    if not data:
        return {"error": "SPU results not found"}

    return data


def aggregate_fabric():
    """Aggregate fabric latency results"""
    loopback = load_json("benchmarks/fabric_latency.json")

    if not loopback:
        return {"error": "fabric results not found"}

    return {
        "loopback": loopback,
        "rdma": None  # RDMA not available in this environment
    }


def generate_notes():
    """Generate summary notes"""
    notes = []

    # Check dynamics
    dynamics_data = load_json("benchmarks/dynamics_determinism.json")
    if dynamics_data:
        all_det = all(r.get("deterministic", False) for r in dynamics_data)
        if not all_det:
            failing = [r["seed"] for r in dynamics_data if not r.get("deterministic", False)]
            notes.append(f"WARNING: Non-deterministic dynamics for seeds: {failing}")

    # Check persistence
    baseline = load_json("benchmarks/persistence_results.json")
    if baseline and baseline.get("stats", {}).get("failed", 0) > 0:
        notes.append(f"Persistence failures: {baseline['stats']['failed']}")

    # Check batching improvement
    batched = load_json("benchmarks/persistence_results_batch5.json")
    if baseline and batched:
        median_improvement = round((1 - batched["stats"]["median_ms"] / baseline["stats"]["median_ms"]) * 100, 1)
        if median_improvement > 50:
            notes.append(f"Batching improved median latency by {median_improvement}%")

    if not notes:
        notes.append("All benchmarks passed successfully")

    return "; ".join(notes)


def main():
    import argparse
    parser = argparse.ArgumentParser()
    parser.add_argument("--out", default="benchmarks/results_summary.json")
    args = parser.parse_args()

    # Aggregate all results
    summary = {
        "timestamp": datetime.utcnow().isoformat() + "Z",
        "commit": get_git_commit(),
        "persistence": aggregate_persistence(),
        "dynamics": aggregate_dynamics(),
        "spu": aggregate_spu(),
        "fabric": aggregate_fabric(),
        "notes": generate_notes()
    }

    # Save summary
    with open(args.out, 'w') as f:
        json.dump(summary, f, indent=2)

    print(f"Aggregated results saved to: {args.out}")
    print()
    print("=== Benchmark Summary ===")
    print(f"Timestamp: {summary['timestamp']}")
    print(f"Commit: {summary['commit']}")
    print()

    # Persistence
    if "baseline" in summary["persistence"]:
        baseline = summary["persistence"]["baseline"]
        print("Persistence (baseline):")
        print(f"  Median: {baseline.get('median_ms', 0)} ms")
        print(f"  P95: {baseline.get('p95_ms', 0)} ms")
        print(f"  P99: {baseline.get('p99_ms', 0)} ms")

        if "batched" in summary["persistence"]:
            batched = summary["persistence"]["batched"]["stats"]
            improvement = summary["persistence"]["batched"]["improvement"]
            print(f"\nPersistence (batched, 5ms window):")
            print(f"  Median: {batched.get('median_ms', 0)} ms ({improvement['median_reduction_pct']}% improvement)")
            print(f"  P99: {batched.get('p99_ms', 0)} ms ({improvement['p99_reduction_pct']}% improvement)")

    print()

    # Dynamics
    dynamics = summary["dynamics"]
    print("Dynamics:")
    print(f"  Deterministic: {dynamics.get('deterministic', False)}")
    print(f"  Seeds tested: {dynamics.get('seeds_tested', 0)}")

    print()

    # SPU
    print("SPU:")
    for prim in summary["spu"]:
        print(f"  {prim['primitive']}: {prim['avg_latency_us']} µs")

    print()

    # Fabric
    fabric = summary["fabric"].get("loopback", {})
    print("Fabric (loopback):")
    print(f"  P50: {fabric.get('p50_ms', 0)} ms")
    print(f"  P95: {fabric.get('p95_ms', 0)} ms")
    print(f"  P99: {fabric.get('p99_ms', 0)} ms")

    print()
    print(f"Notes: {summary['notes']}")

    return 0


if __name__ == "__main__":
    import sys
    sys.exit(main())
