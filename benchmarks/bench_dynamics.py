#!/usr/bin/env python3
"""
Dynamics determinism benchmark
"""

import hashlib
import json
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).parent / ".." / "runtime"))

from dynamics.engine import Glyph, DynamicsEngine
from cli import create_glyph


def run_dynamics_sequence(seed, num_steps=100, save_snapshots=True):
    """Run dynamics for num_steps and return sequence of states"""
    # Create initial glyph with seed-based content
    content = f"Dynamics seed {seed}"
    metadata = {
        "energy": 10.0,
        "activation_count": 0,
        "last_update_time": 0
    }

    glyph_id, glyph_data = create_glyph.create_glyph(content, metadata)
    glyph = Glyph.from_dict(glyph_data)

    # Create engine
    engine = DynamicsEngine(activation_threshold=1.0, decay_rate=0.1)

    # Run steps and collect states
    states = []
    snapshots_dir = Path("persistence") / "snapshots" / f"seed_{seed}"

    if save_snapshots:
        snapshots_dir.mkdir(parents=True, exist_ok=True)

    for step in range(num_steps):
        # Apply dynamics step
        glyph, step_info = engine.step(glyph, time_delta=1)

        # Record state
        state = {
            "step": step,
            "energy": glyph.energy,
            "activated": glyph.activation_count > 0,
            "activation_count": glyph.activation_count
        }
        states.append(state)

        # Save snapshot if requested
        if save_snapshots and step % 10 == 0:  # Save every 10th step
            snapshot_path = snapshots_dir / f"glyph_{glyph_id}.step_{step}.json"
            with open(snapshot_path, 'w') as f:
                json.dump(glyph.to_dict(), f, indent=2)

    return states, glyph_id


def test_determinism(num_seeds=10, num_steps=100):
    """Test that dynamics is deterministic across multiple runs"""
    results = []

    print(f"Testing determinism across {num_seeds} seeds, {num_steps} steps each...")

    for seed in range(num_seeds):
        print(f"  Seed {seed}...")

        # Run twice with same seed
        states1, glyph_id1 = run_dynamics_sequence(seed, num_steps, save_snapshots=(seed == 0))
        states2, glyph_id2 = run_dynamics_sequence(seed, num_steps, save_snapshots=False)

        # Check if IDs match (same content = same ID)
        ids_match = (glyph_id1 == glyph_id2)

        # Check if state sequences match
        diffs = []
        for i, (s1, s2) in enumerate(zip(states1, states2)):
            if s1 != s2:
                diffs.append({
                    "step": i,
                    "state1": s1,
                    "state2": s2
                })

        deterministic = (ids_match and len(diffs) == 0)

        results.append({
            "seed": seed,
            "deterministic": deterministic,
            "ids_match": ids_match,
            "diffs": diffs[:5]  # Only include first 5 diffs
        })

        if not deterministic:
            print(f"    WARNING: Non-deterministic behavior detected!")
            print(f"    IDs match: {ids_match}")
            print(f"    Diffs found: {len(diffs)}")

    return results


def main():
    import argparse
    parser = argparse.ArgumentParser()
    parser.add_argument("--num-seeds", type=int, default=10)
    parser.add_argument("--num-steps", type=int, default=100)
    parser.add_argument("--output", default="benchmarks/dynamics_determinism.json")
    args = parser.parse_args()

    # Run determinism test
    results = test_determinism(args.num_seeds, args.num_steps)

    # Analyze
    all_deterministic = all(r["deterministic"] for r in results)
    num_failed = sum(1 for r in results if not r["deterministic"])

    print(f"\nResults:")
    print(f"  Seeds tested: {args.num_seeds}")
    print(f"  Steps per seed: {args.num_steps}")
    print(f"  All deterministic: {all_deterministic}")
    print(f"  Failed seeds: {num_failed}")

    if not all_deterministic:
        print(f"\n  Failed seeds:")
        for r in results:
            if not r["deterministic"]:
                print(f"    Seed {r['seed']}: {len(r['diffs'])} diffs")

    # Save results
    with open(args.output, 'w') as f:
        json.dump(results, f, indent=2)

    print(f"\nResults saved to: {args.output}")

    return 0 if all_deterministic else 1


if __name__ == "__main__":
    sys.exit(main())
