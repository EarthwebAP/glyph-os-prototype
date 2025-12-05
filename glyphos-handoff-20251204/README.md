# GlyphOS Hardware Offload Prototype - Handoff Package

**Date:** 2025-12-04  
**Release:** glyphos-node-alpha v0.1.0  
**Previous Stage:** software-stage-complete

## Overview

This handoff package contains all components needed for the hardware offload prototype stage:

1. **FPGA HLS Merge Accelerator** - Xilinx Vivado HLS implementation
2. **DMA Contract** - Host-FPGA interface specification with verification protocol
3. **Rust Node Runtime** - glyphd (persistence) + glyph-spu (SPU offload service)
4. **FreeBSD Overlay** - rc.d scripts and ISO build tooling
5. **CI Parity Check** - Verification that HLS matches software reference
6. **Documentation** - API specifications and integration guides

## Quick Start

### Prerequisites

```bash
# Vivado/Vitis HLS 2023.2
source /tools/Xilinx/Vivado/2023.2/settings64.sh

# Rust 1.72+
rustc --version

# QEMU 7.x (for ISO testing)
qemu-system-x86_64 --version
```

### Task Execution Order

Follow tasks in the specified sequence:

#### 1. FPGA HLS Synthesis

```bash
cd fpga/hls
vivado_hls -f run_hls.tcl  # Create this TCL script
# Expected output: merge_accel_hls.v, synthesis report
```

**Inputs:**
- `fpga/hls/merge_accel_hls.cpp`
- `fpga/docs/dma_contract.json`
- `fpga/sim/test_vectors/merge_inputs.json`

**Outputs:**
- `fpga/hls/merge_accel_hls.v` (RTL)
- `merge_accel_hls_synth_report.txt`
- `benchmarks/merge_hw_sim.json` (simulation results)

**Acceptance:** Simulation results match `../benchmarks/merge_ref_results.json`

#### 2. DMA Contract Validation

```bash
python3 ci/check_parity.py \
  --hw benchmarks/merge_hw_sim.json \
  --ref ../benchmarks/merge_ref_results.json
```

**Expected Output:** `PARITY OK` in `ci/parity_report.json`

#### 3. Node Runtime Integration

```bash
# Build Rust services
cd runtime/rust
cargo build --release

# Start glyphd
./target/release/glyphd &

# Start glyph-spu
./target/release/glyph-spu &

# Test endpoints
curl http://localhost:8080/health  # "glyphd OK"
curl http://localhost:8081/health  # "glyph-spu OK"

# Create glyph
curl -X POST http://localhost:8080/glyphs \
  -H "Content-Type: application/json" \
  -d '{"content": "test", "metadata": {}}'

# Test merge
curl -X POST http://localhost:8081/offload/merge \
  -H "Content-Type: application/json" \
  -d @../fpga/sim/test_vectors/merge_inputs.json
```

**Acceptance:** glyphd returns commit_id, glyph-spu returns merged_state matching reference

#### 4. FreeBSD ISO Staging

```bash
cd freebsd
./build_iso.sh
# Output: glyphos-freebsd-0.1.0.iso

# Boot in QEMU
qemu-system-x86_64 \
  -cdrom glyphos-freebsd-0.1.0.iso \
  -m 2G \
  -net nic \
  -net user,hostfwd=tcp::8080-:8080,hostfwd=tcp::8081-:8081
```

**Acceptance:** VM boots, services start, host can curl endpoints

#### 5. Parity and CI

```bash
# Run parity check
python3 ci/check_parity.py

# Verify CI workflow
cat .github/workflows/perf.yml
```

**Acceptance:** Parity OK, CI configured to gate merges

## Directory Structure

```
glyphos-handoff-20251204/
├── fpga/
│   ├── hls/merge_accel_hls.cpp          # HLS implementation
│   ├── docs/dma_contract.json            # DMA interface spec
│   └── sim/test_vectors/merge_inputs.json # Test vectors
├── runtime/
│   ├── rust/
│   │   ├── glyphd/                       # Node daemon
│   │   └── glyph-spu/                    # SPU service
│   └── wasm/add-int.wasm.wat             # WASM stub
├── freebsd/
│   ├── overlay/usr/local/etc/rc.d/       # rc.d scripts
│   └── build_iso.sh                      # ISO builder
├── ci/
│   ├── check_parity.py                   # Parity verification
│   └── perf_baseline.json                # Performance baselines
├── docs/
│   ├── spu_offload_api.md                # SPU API spec
│   └── persistence_api.md                # Persistence API spec
├── .github/workflows/perf.yml            # CI workflow
├── release_manifest.glyphos-node-alpha.json # Artifact manifest
└── README.md                             # This file
```

## Component Status

| Component | Status | Description |
|-----------|--------|-------------|
| FPGA HLS | Simulated | Ready for Vivado synthesis |
| DMA Contract | Specified | JSON contract with verification protocol |
| Rust Runtime | Ready | Builds with cargo, APIs functional |
| FreeBSD Overlay | Staged | rc.d scripts ready, ISO build placeholder |
| CI Parity | Ready | Verification script complete |
| Documentation | Complete | API specs and integration guides |

## Performance Targets

From previous software stage:

| Metric | Software Baseline | FPGA Target |
|--------|-------------------|-------------|
| SPU Merge (Python) | 5.33 µs | - |
| SPU Merge (C++) | 0.57 µs | - |
| FPGA (1 lane) | - | 355 ns |
| FPGA (16 lanes) | - | 3.2M ops/sec |
| Persistence P99 | 12.5 ms (sync) | - |
| Fabric P50 | 14.7 µs (loopback) | - |

## Verification Checklist

- [ ] FPGA HLS synthesizes without errors
- [ ] Simulation merged_state matches software reference (check_parity.py)
- [ ] glyphd and glyph-spu build and run
- [ ] POST /glyphs returns commit_id
- [ ] POST /offload/merge returns correct merged_state
- [ ] FreeBSD ISO boots in QEMU
- [ ] Services start on boot
- [ ] CI parity check passes

## Next Tags

Upon successful completion:

- `fpga-merge-proto-v1` - After HLS synthesis and simulation
- `runtime-integration-v1` - After Rust runtime integration
- `freebsd-iso-v1` - After ISO boots successfully

## References

- Software benchmarks: `../benchmarks/results_summary.json`
- Previous release: `software-stage-complete` (tag)
- Repository: https://github.com/EarthwebAP/glyph-os-prototype

---

**Handoff Package Version:** 1.0  
**Created:** 2025-12-04  
**Previous Commit:** 4ab82a6
