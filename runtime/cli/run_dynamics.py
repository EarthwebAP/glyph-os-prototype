#!/usr/bin/env python3
"""
run_dynamics.py - Apply dynamics engine to a persisted glyph
"""

import argparse
import json
import sys
from pathlib import Path

# Add parent directory to path
sys.path.insert(0, str(Path(__file__).parent / ".."))

from dynamics.engine import Glyph, DynamicsEngine
import query_glyph
import create_glyph


def main():
    parser = argparse.ArgumentParser(description="Run dynamics engine on a glyph")
    parser.add_argument("glyph_id", help="ID of the glyph to process")
    parser.add_argument("--time-delta", type=int, default=1, help="Time steps to simulate")
    parser.add_argument("--activation-threshold", type=float, default=1.0, help="Activation threshold")
    parser.add_argument("--decay-rate", type=float, default=0.1, help="Decay rate (0.0-1.0)")
    parser.add_argument("--save", action="store_true", help="Save updated glyph back to persistence")
    parser.add_argument("--verbose", "-v", action="store_true", help="Verbose output")

    args = parser.parse_args()

    # Load glyph
    glyph_data = query_glyph.query_glyph(args.glyph_id)
    if glyph_data is None:
        print(f"Error: Glyph '{args.glyph_id}' not found", file=sys.stderr)
        sys.exit(1)

    # Create Glyph object
    glyph = Glyph.from_dict(glyph_data)

    if args.verbose:
        print(f"Initial state:", file=sys.stderr)
        print(f"  Energy: {glyph.energy}", file=sys.stderr)
        print(f"  Activation count: {glyph.activation_count}", file=sys.stderr)
        print(f"  Last update: {glyph.last_update_time}", file=sys.stderr)

    # Create engine
    engine = DynamicsEngine(
        activation_threshold=args.activation_threshold,
        decay_rate=args.decay_rate
    )

    # Run dynamics step
    updated_glyph, step_info = engine.step(glyph, time_delta=args.time_delta)

    if args.verbose:
        print(f"\nDynamics step (Δt={args.time_delta}):", file=sys.stderr)
        print(f"  Energy after decay: {step_info['energy_after_decay']:.4f}", file=sys.stderr)
        print(f"  Activated: {step_info['activated']}", file=sys.stderr)
        print(f"  Final energy: {step_info['final_energy']:.4f}", file=sys.stderr)
        print(f"  Activation count: {step_info['activation_count']}", file=sys.stderr)

    # Save if requested
    if args.save:
        updated_data = updated_glyph.to_dict()
        file_path = create_glyph.save_glyph(updated_glyph.id, updated_data)
        if args.verbose:
            print(f"\nSaved updated glyph to: {file_path}", file=sys.stderr)

    # Output updated glyph
    print(json.dumps(updated_glyph.to_dict(), indent=2))

    return 0


if __name__ == "__main__":
    sys.exit(main())
