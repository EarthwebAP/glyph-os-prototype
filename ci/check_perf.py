#!/usr/bin/env python3
"""
Performance regression check

Compares current benchmark results against baseline and fails if
regressions exceed configured thresholds.

Usage:
  python3 ci/check_perf.py \
    --baseline ci/perf_baseline.json \
    --current benchmarks/spu_results_ci.json \
    --current-persistence benchmarks/persistence_ci.json
"""

import argparse
import json
import sys


class Colors:
    """ANSI color codes for terminal output"""

    GREEN = "\033[92m"
    YELLOW = "\033[93m"
    RED = "\033[91m"
    RESET = "\033[0m"
    BOLD = "\033[1m"


def load_json(path):
    """Load JSON file"""
    try:
        with open(path, "r") as f:
            return json.load(f)
    except FileNotFoundError:
        print(f"Error: File not found: {path}")
        return None
    except json.JSONDecodeError as e:
        print(f"Error: Invalid JSON in {path}: {e}")
        return None


def check_spu_regression(baseline, current, thresholds):
    """
    Check SPU benchmark for regressions

    Returns: (passed, messages)
    """
    passed = True
    messages = []

    # Extract metrics
    baseline_latency = baseline["spu"]["merge"]["avg_latency_us"]
    baseline_ops = baseline["spu"]["merge"]["ops_per_sec"]

    # Current is a list, get merge primitive
    current_merge = None
    for prim in current:
        if prim["primitive"] == "merge":
            current_merge = prim
            break

    if not current_merge:
        return False, ["Error: merge primitive not found in current results"]

    current_latency = current_merge["avg_latency_us"]
    current_ops = current_merge["ops_per_sec"]

    # Check latency regression
    latency_increase_pct = ((current_latency - baseline_latency) / baseline_latency) * 100

    if latency_increase_pct > thresholds["spu_latency_increase_pct"]:
        passed = False
        messages.append(
            f"{Colors.RED}✗ SPU latency regression: {latency_increase_pct:.1f}% increase "
            f"({baseline_latency:.2f} → {current_latency:.2f} µs){Colors.RESET}"
        )
    elif latency_increase_pct > thresholds["spu_latency_increase_pct"] / 2:
        messages.append(
            f"{Colors.YELLOW}⚠ SPU latency warning: {latency_increase_pct:.1f}% increase "
            f"({baseline_latency:.2f} → {current_latency:.2f} µs){Colors.RESET}"
        )
    else:
        messages.append(
            f"{Colors.GREEN}✓ SPU latency OK: {latency_increase_pct:+.1f}% "
            f"({baseline_latency:.2f} → {current_latency:.2f} µs){Colors.RESET}"
        )

    # Check throughput regression
    ops_decrease_pct = ((baseline_ops - current_ops) / baseline_ops) * 100

    if ops_decrease_pct > thresholds["spu_throughput_decrease_pct"]:
        passed = False
        messages.append(
            f"{Colors.RED}✗ SPU throughput regression: {ops_decrease_pct:.1f}% decrease "
            f"({baseline_ops:,} → {current_ops:,} ops/sec){Colors.RESET}"
        )
    elif ops_decrease_pct > thresholds["spu_throughput_decrease_pct"] / 2:
        messages.append(
            f"{Colors.YELLOW}⚠ SPU throughput warning: {ops_decrease_pct:.1f}% decrease "
            f"({baseline_ops:,} → {current_ops:,} ops/sec){Colors.RESET}"
        )
    else:
        messages.append(
            f"{Colors.GREEN}✓ SPU throughput OK: {ops_decrease_pct:+.1f}% "
            f"({baseline_ops:,} → {current_ops:,} ops/sec){Colors.RESET}"
        )

    return passed, messages


def check_persistence_regression(baseline, current, thresholds):
    """
    Check persistence benchmark for regressions

    Returns: (passed, messages)
    """
    passed = True
    messages = []

    # Extract metrics
    baseline_p99 = baseline["persistence"]["p99_ms"]
    current_p99 = current["stats"]["p99_ms"]

    # Check P99 regression
    p99_increase_pct = ((current_p99 - baseline_p99) / baseline_p99) * 100

    if p99_increase_pct > thresholds["persistence_p99_increase_pct"]:
        passed = False
        messages.append(
            f"{Colors.RED}✗ Persistence P99 regression: {p99_increase_pct:.1f}% increase "
            f"({baseline_p99:.2f} → {current_p99:.2f} ms){Colors.RESET}"
        )
    elif p99_increase_pct > thresholds["persistence_p99_increase_pct"] / 2:
        messages.append(
            f"{Colors.YELLOW}⚠ Persistence P99 warning: {p99_increase_pct:.1f}% increase "
            f"({baseline_p99:.2f} → {current_p99:.2f} ms){Colors.RESET}"
        )
    else:
        messages.append(
            f"{Colors.GREEN}✓ Persistence P99 OK: {p99_increase_pct:+.1f}% "
            f"({baseline_p99:.2f} → {current_p99:.2f} ms){Colors.RESET}"
        )

    return passed, messages


def main():
    parser = argparse.ArgumentParser(description="Check for performance regressions")
    parser.add_argument(
        "--baseline", required=True, help="Baseline performance JSON file"
    )
    parser.add_argument("--current", required=True, help="Current SPU results JSON")
    parser.add_argument(
        "--current-persistence", help="Current persistence results JSON"
    )
    parser.add_argument(
        "--spu-latency-threshold",
        type=float,
        default=20.0,
        help="Max SPU latency increase %% (default: 20)",
    )
    parser.add_argument(
        "--spu-throughput-threshold",
        type=float,
        default=20.0,
        help="Max SPU throughput decrease %% (default: 20)",
    )
    parser.add_argument(
        "--persistence-p99-threshold",
        type=float,
        default=50.0,
        help="Max persistence P99 increase %% (default: 50)",
    )

    args = parser.parse_args()

    print(f"{Colors.BOLD}=== Performance Regression Check ==={Colors.RESET}\n")

    # Load baseline
    baseline = load_json(args.baseline)
    if not baseline:
        return 1

    # Load current results
    current_spu = load_json(args.current)
    if not current_spu:
        return 1

    # Configure thresholds
    thresholds = {
        "spu_latency_increase_pct": args.spu_latency_threshold,
        "spu_throughput_decrease_pct": args.spu_throughput_threshold,
        "persistence_p99_increase_pct": args.persistence_p99_threshold,
    }

    print(f"Thresholds:")
    print(f"  SPU latency increase: {thresholds['spu_latency_increase_pct']}%")
    print(
        f"  SPU throughput decrease: {thresholds['spu_throughput_decrease_pct']}%"
    )
    print(
        f"  Persistence P99 increase: {thresholds['persistence_p99_increase_pct']}%"
    )
    print()

    all_passed = True

    # Check SPU
    print(f"{Colors.BOLD}SPU Merge Primitive:{Colors.RESET}")
    spu_passed, spu_messages = check_spu_regression(baseline, current_spu, thresholds)
    for msg in spu_messages:
        print(f"  {msg}")
    print()

    all_passed = all_passed and spu_passed

    # Check persistence (if provided)
    if args.current_persistence:
        current_persistence = load_json(args.current_persistence)
        if current_persistence:
            print(f"{Colors.BOLD}Persistence:{Colors.RESET}")
            persist_passed, persist_messages = check_persistence_regression(
                baseline, current_persistence, thresholds
            )
            for msg in persist_messages:
                print(f"  {msg}")
            print()

            all_passed = all_passed and persist_passed

    # Final result
    print(f"{Colors.BOLD}Overall Result:{Colors.RESET}")
    if all_passed:
        print(f"  {Colors.GREEN}{Colors.BOLD}✓ All checks passed{Colors.RESET}")
        return 0
    else:
        print(
            f"  {Colors.RED}{Colors.BOLD}✗ Performance regression detected{Colors.RESET}"
        )
        return 1


if __name__ == "__main__":
    sys.exit(main())
