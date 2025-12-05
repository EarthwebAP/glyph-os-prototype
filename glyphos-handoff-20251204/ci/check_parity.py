#!/usr/bin/env python3
"""
HLS Hardware Simulation Parity Check

Compares HLS simulation results against software reference to verify
functional correctness before FPGA deployment.

Checks:
  - benchmarks/merge_hw_sim.json vs benchmarks/merge_ref_results.json
  - Verifies merged_state fields match (id, content, energy)

Output:
  - ci/parity_report.json with PARITY OK/FAIL status
  - Exit 0 if parity OK, exit 1 if fail

Usage:
  python3 ci/check_parity.py
"""

import argparse
import json
import sys
from pathlib import Path


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
        print(f"{Colors.RED}Error: File not found: {path}{Colors.RESET}")
        return None
    except json.JSONDecodeError as e:
        print(f"{Colors.RED}Error: Invalid JSON in {path}: {e}{Colors.RESET}")
        return None


def save_json(path, data):
    """Save JSON file"""
    try:
        with open(path, "w") as f:
            json.dump(data, f, indent=2)
        return True
    except Exception as e:
        print(f"{Colors.RED}Error saving {path}: {e}{Colors.RESET}")
        return False


def compare_merged_states(hw_result, ref_result):
    """
    Compare merged_state fields between HLS simulation and software reference.

    Returns: (match, mismatches)
      match: bool - True if all fields match
      mismatches: list - list of mismatch descriptions
    """
    mismatches = []

    # Check if merged_state exists in both
    if "merged_state" not in hw_result:
        mismatches.append("HW simulation missing 'merged_state' field")
        return False, mismatches

    if "merged_state" not in ref_result:
        mismatches.append("Reference missing 'merged_state' field")
        return False, mismatches

    hw_state = hw_result["merged_state"]
    ref_state = ref_result["merged_state"]

    # Check critical fields
    fields_to_check = ["id", "content", "energy"]

    for field in fields_to_check:
        if field not in hw_state:
            mismatches.append(f"HW simulation missing field: {field}")
            continue

        if field not in ref_state:
            mismatches.append(f"Reference missing field: {field}")
            continue

        hw_val = hw_state[field]
        ref_val = ref_state[field]

        # Special handling for floating-point energy values
        if field == "energy" and isinstance(hw_val, (int, float)) and isinstance(ref_val, (int, float)):
            # Allow small floating-point tolerance (0.01%)
            tolerance = abs(ref_val) * 0.0001 if ref_val != 0 else 1e-9
            if abs(hw_val - ref_val) > tolerance:
                mismatches.append(
                    f"Field '{field}' mismatch: HW={hw_val}, REF={ref_val}, "
                    f"delta={abs(hw_val - ref_val):.6f}"
                )
        else:
            # Exact match for id and content
            if hw_val != ref_val:
                mismatches.append(
                    f"Field '{field}' mismatch: HW={hw_val}, REF={ref_val}"
                )

    return len(mismatches) == 0, mismatches


def check_parity(hw_sim_path, ref_path):
    """
    Check parity between HLS simulation and software reference.

    Returns: (passed, report_data)
    """
    print(f"{Colors.BOLD}=== HLS Simulation Parity Check ==={Colors.RESET}\n")

    # Load files
    print(f"Loading HLS simulation results: {hw_sim_path}")
    hw_result = load_json(hw_sim_path)
    if hw_result is None:
        return False, {
            "status": "FAIL",
            "error": f"Failed to load HLS simulation file: {hw_sim_path}"
        }

    print(f"Loading software reference: {ref_path}")
    ref_result = load_json(ref_path)
    if ref_result is None:
        return False, {
            "status": "FAIL",
            "error": f"Failed to load reference file: {ref_path}"
        }

    print()

    # Compare merged states
    print(f"{Colors.BOLD}Comparing merged_state fields:{Colors.RESET}")
    match, mismatches = compare_merged_states(hw_result, ref_result)

    report_data = {
        "hw_simulation_file": str(hw_sim_path),
        "reference_file": str(ref_path),
        "timestamp": None,  # Could add timestamp if needed
    }

    if match:
        print(f"  {Colors.GREEN}✓ id: MATCH{Colors.RESET}")
        print(f"  {Colors.GREEN}✓ content: MATCH{Colors.RESET}")
        print(f"  {Colors.GREEN}✓ energy: MATCH{Colors.RESET}")
        print()

        report_data["status"] = "PARITY OK"
        report_data["result"] = "All merged_state fields match between HLS simulation and software reference"

        print(f"{Colors.BOLD}Overall Result:{Colors.RESET}")
        print(f"  {Colors.GREEN}{Colors.BOLD}✓ PARITY OK{Colors.RESET}")
        print(f"  HLS simulation matches software reference")

        return True, report_data
    else:
        print(f"{Colors.RED}✗ Parity check FAILED{Colors.RESET}")
        print(f"{Colors.RED}Mismatches detected:{Colors.RESET}")
        for mismatch in mismatches:
            print(f"  - {mismatch}")
        print()

        report_data["status"] = "PARITY FAIL"
        report_data["result"] = "Mismatches detected between HLS simulation and software reference"
        report_data["mismatches"] = mismatches

        print(f"{Colors.BOLD}Overall Result:{Colors.RESET}")
        print(f"  {Colors.RED}{Colors.BOLD}✗ PARITY FAIL{Colors.RESET}")
        print(f"  HLS simulation does NOT match software reference")

        return False, report_data


def main():
    parser = argparse.ArgumentParser(
        description="Check parity between HLS simulation and software reference"
    )
    parser.add_argument(
        "--hw-sim",
        default="benchmarks/merge_hw_sim.json",
        help="Path to HLS simulation results (default: benchmarks/merge_hw_sim.json)",
    )
    parser.add_argument(
        "--reference",
        default="benchmarks/merge_ref_results.json",
        help="Path to software reference results (default: benchmarks/merge_ref_results.json)",
    )
    parser.add_argument(
        "--output",
        default="ci/parity_report.json",
        help="Path to output parity report (default: ci/parity_report.json)",
    )

    args = parser.parse_args()

    # Check parity
    passed, report_data = check_parity(args.hw_sim, args.reference)

    # Save report
    print()
    print(f"Saving parity report to: {args.output}")
    if save_json(args.output, report_data):
        print(f"{Colors.GREEN}Report saved successfully{Colors.RESET}")
    else:
        print(f"{Colors.RED}Failed to save report{Colors.RESET}")

    # Exit with appropriate code
    return 0 if passed else 1


if __name__ == "__main__":
    sys.exit(main())
