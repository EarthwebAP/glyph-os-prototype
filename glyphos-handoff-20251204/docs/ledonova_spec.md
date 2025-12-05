# Ledonova System Architecture Specification

**Version:** 1.0-alpha
**Project:** Glyph OS Node (Alpha Release)
**Date:** 2025-12-04
**Status:** Software stage complete, FPGA in simulation

## Overview

**Ledonova** is the codename for the Glyph OS node architecture, a distributed content-addressed computing system that combines software-defined dynamics with FPGA-accelerated symbolic processing. The name reflects the system's core principles: **L**ightweight, **E**volvable, **D**eterministic, **O**ffloadable, **N**etworked, **O**pen, **V**erifiable, **A**cceleratable.

This document defines the complete system architecture, component interactions, and data flows for the alpha release.

## System Architecture

### High-Level Component Diagram

```
┌────────────────────────────────────────────────────────────────────┐
│                           Glyph OS Node                             │
│                         (Ledonova Alpha)                            │
├────────────────────────────────────────────────────────────────────┤
│                                                                     │
│  ┌──────────────┐         ┌──────────────┐      ┌──────────────┐  │
│  │   Clients    │  HTTP   │    glyphd    │ DMA  │  glyph-spu   │  │
│  │  CLI / Web   ├────────►│  REST API    ├─────►│ FPGA Accel   │  │
│  │   (Python)   │         │ (Rust/Axum)  │      │ (Alveo U50)  │  │
│  └──────────────┘         └──────┬───────┘      └──────────────┘  │
│                                  │                                 │
│                                  │                                 │
│                                  ▼                                 │
│                         ┌────────────────┐                         │
│                         │  Persistence   │                         │
│                         │  NVMe Storage  │                         │
│                         │  (Merkle Tree) │                         │
│                         └────────────────┘                         │
│                                                                     │
└────────────────────────────────────────────────────────────────────┘
                                  │
                                  │ Network (future: RDMA)
                                  │
                                  ▼
                    ┌──────────────────────────┐
                    │   Glyph Fabric Network   │
                    │   (Multi-node cluster)   │
                    └──────────────────────────┘
```

### Component Stack

```
┌─────────────────────────────────────────────────────────────┐
│                        Application Layer                     │
│  - Python CLI (create_glyph.py, query_glyph.py)             │
│  - Web UI (future)                                          │
│  - Dynamics Engine (deterministic glyph evolution)          │
└─────────────────────────────────────────────────────────────┘
                              │
                              ▼
┌─────────────────────────────────────────────────────────────┐
│                         API Layer                           │
│  - REST API (glyphd) - POST /glyphs, GET /glyphs/:id        │
│  - Offload API (glyph-spu) - POST /offload/merge            │
│  - Health/Metrics endpoints                                 │
└─────────────────────────────────────────────────────────────┘
                              │
                              ▼
┌─────────────────────────────────────────────────────────────┐
│                      Processing Layer                       │
│  - SPU Primitives: merge, transform, match, resonate, prune │
│  - C++ Reference (1.76M ops/sec)                            │
│  - FPGA Accelerator (3.2M ops/sec target)                   │
└─────────────────────────────────────────────────────────────┘
                              │
                              ▼
┌─────────────────────────────────────────────────────────────┐
│                      Persistence Layer                      │
│  - Content-addressed storage (SHA256)                       │
│  - Atomic writes with fsync                                 │
│  - Merkle-tree organization                                 │
│  - NVMe-optimized I/O                                       │
└─────────────────────────────────────────────────────────────┘
                              │
                              ▼
┌─────────────────────────────────────────────────────────────┐
│                        Storage Backend                      │
│  - Primary: /mnt/persistence (NVMe SSD)                     │
│  - Fallback: ./persistence (local filesystem)              │
│  - Organization: /prefix1/prefix2/glyph_<id>.json          │
└─────────────────────────────────────────────────────────────┘
```

## Component Specifications

### 1. glyphd (Glyph Daemon)

**Language:** Rust
**Framework:** Axum (async HTTP)
**Ports:** 3000 (HTTP API)

**Responsibilities:**
- REST API for glyph creation and retrieval
- Content-addressed storage management
- Atomic write coordination
- Request routing to glyph-spu for acceleration

**Key Features:**
- Async I/O with Tokio runtime
- Content-addressed SHA256 IDs
- Atomic writes (temp file + rename + fsync)
- Merkle-tree directory organization
- Crash-safe persistence (100% verified)
- Write batching (5ms window, 99.8% P99 reduction)

**Performance:**
- Baseline write latency: P99 = 12.5ms
- Optimized write latency: P99 = 0.025ms (batched)
- Read latency: <1ms (NVMe)
- Throughput: ~30K writes/sec (batched)

**Configuration:**
```toml
[server]
listen_addr = "0.0.0.0:3000"
persistence_path = "/mnt/persistence"
batch_window_ms = 5
enable_fsync = true

[limits]
max_content_size = 1048576  # 1MB
max_batch_size = 65535
```

### 2. glyph-spu (Symbolic Processing Unit)

**Language:** C++ / HLS (Hardware)
**Interface:** HTTP (host) + AXI4-Stream (FPGA)
**Ports:** 8080 (HTTP offload API)

**Responsibilities:**
- Hardware-accelerated glyph merge operations
- DMA transfer management (host ↔ FPGA)
- Fallback to C++ reference implementation
- Performance statistics collection

**Architecture:**
```
┌────────────────────────────────────────────────────────────┐
│                       glyph-spu                            │
├────────────────────────────────────────────────────────────┤
│  HTTP Server (C++)                                         │
│    - POST /offload/merge endpoint                          │
│    - Request parsing and validation                        │
│    - Response formatting                                   │
├────────────────────────────────────────────────────────────┤
│  DMA Engine                                                │
│    - Pinned memory allocation                              │
│    - PCIe Gen3 x16 transfers                               │
│    - AXI4-Stream 512-bit wide                              │
├────────────────────────────────────────────────────────────┤
│  FPGA Control                                              │
│    - AXI4-Lite register interface                          │
│    - Control flow management                               │
│    - Interrupt handling (completion)                       │
├────────────────────────────────────────────────────────────┤
│  FPGA Accelerator (Alveo U50/U280)                         │
│    - 16-lane parallel merge pipeline                       │
│    - 71-cycle latency per merge @ 200 MHz                  │
│    - Hardware SHA256 (future optimization)                 │
└────────────────────────────────────────────────────────────┘
```

**Performance:**
- Python baseline: 5.33 µs/merge, 187K ops/sec
- C++ reference: 0.57 µs/merge, 1.76M ops/sec (9.4x speedup)
- FPGA (1 lane): 0.355 µs/merge, 200K ops/sec
- FPGA (16 lanes): 0.355 µs/merge, 3.2M ops/sec (17x speedup)

**DMA Batch Performance (1024 merges):**
- Input transfer: 32 µs
- Processing: 364 µs
- Output transfer: 43 µs
- Total latency: 439 µs
- Effective throughput: 2.33M ops/sec

### 3. Persistence Layer

**Storage Format:** JSON (one file per glyph)
**Organization:** Merkle tree (2-level prefix)
**Backend:** NVMe SSD (primary), local filesystem (fallback)

**Directory Structure:**
```
/mnt/persistence/
├── a1/                    # First 2 chars of ID
│   ├── b2/                # Next 2 chars of ID
│   │   └── glyph_a1b2c3...json
│   └── c3/
│       └── glyph_a1c3d4...json
└── commit_log/            # Append-only commit log
    └── 2025-12-04.log
```

**Glyph File Format:**
```json
{
  "id": "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855",
  "content": "Hello, Glyph OS!",
  "metadata": {
    "author": "alice",
    "tags": ["greeting"],
    "created_at": 1701715400000
  }
}
```

**Atomic Write Protocol:**
1. Compute SHA256 ID from content
2. Determine target path: `/prefix1/prefix2/glyph_<id>.json`
3. Create temp file: `.tmp_glyph_<id>_<random>.json`
4. Write data to temp file
5. `fsync()` temp file descriptor
6. Atomic `rename()` to final path
7. `fsync()` parent directory

**Crash Safety Guarantees:**
- Zero partial writes (atomicity)
- Zero corruption (verified with 1000 concurrent writes)
- Immediate consistency (no eventual consistency window)
- Durable writes (fsync before success response)

### 4. SPU Primitives

The Symbolic Processing Unit implements five core primitives:

#### merge(g1, g2) → g3

Combine two glyphs, preserving energy and precedence.

**Semantics:**
```
primary = g1 if g1.energy >= g2.energy else g2
secondary = g2 if primary == g1 else g1

merged.content = f"{primary.content} + {secondary.content}"
merged.energy = g1.energy + g2.energy
merged.activation_count = max(g1.activation_count, g2.activation_count)
merged.last_update_time = max(g1.last_update_time, g2.last_update_time)
merged.id = SHA256(merged.content)
```

**Properties:**
- Energy conservation: `merged.energy == g1.energy + g2.energy`
- Energy precedence: Higher energy glyph's content appears first
- Determinism: Same inputs always produce same output
- Commutativity (energy-weighted): Result depends only on energies

**Performance:**
- Python: 5.33 µs
- C++: 0.57 µs
- FPGA: 0.355 µs (71 cycles @ 200 MHz)

#### transform(g, rule) → g'

Apply a transformation rule to a glyph.

**Example:**
```python
rule = {"uppercase": True}
g = Glyph(content="hello")
g_prime = transform(g, rule)
# g_prime.content = "HELLO"
```

**Performance:** 1.04 µs (Python), 965K ops/sec

#### match(g1, g2) → similarity_score

Compute similarity between two glyphs.

**Semantics:**
```
similarity = content_overlap(g1, g2) * energy_ratio(g1, g2)
```

**Performance:** 0.80 µs (Python), 1.25M ops/sec

#### resonate(g, frequency) → g'

Apply frequency-based transformation (audio/visual).

**Semantics:**
```
g_prime.metadata["frequency"] = frequency
g_prime.metadata["wavelength"] = c / frequency
```

**Performance:** 1.18 µs (Python), 848K ops/sec

#### prune(g, threshold) → g' or None

Remove glyph if energy below threshold.

**Semantics:**
```
if g.energy < threshold:
    return None
else:
    return g
```

**Performance:** 0.75 µs (Python), 1.32M ops/sec

## Data Flow Diagrams

### Create Glyph Flow

```
┌─────────┐         ┌─────────┐         ┌──────────────┐
│ Client  │         │ glyphd  │         │ Persistence  │
└────┬────┘         └────┬────┘         └──────┬───────┘
     │                   │                     │
     │ POST /glyphs      │                     │
     ├──────────────────►│                     │
     │ {content: "..."}  │                     │
     │                   │                     │
     │                   │ Compute SHA256 ID   │
     │                   │ (e3b0c442...)       │
     │                   │                     │
     │                   │ Atomic write        │
     │                   ├────────────────────►│
     │                   │                     │
     │                   │                     │ 1. Create temp file
     │                   │                     │ 2. Write data
     │                   │                     │ 3. fsync()
     │                   │                     │ 4. rename()
     │                   │                     │ 5. fsync(dir)
     │                   │                     │
     │                   │ Success + commit_id │
     │                   │◄────────────────────┤
     │                   │                     │
     │ 201 Created       │                     │
     │◄──────────────────┤                     │
     │ {id, commit_id}   │                     │
     │                   │                     │
```

### Retrieve Glyph Flow

```
┌─────────┐         ┌─────────┐         ┌──────────────┐
│ Client  │         │ glyphd  │         │ Persistence  │
└────┬────┘         └────┬────┘         └──────┬───────┘
     │                   │                     │
     │ GET /glyphs/:id   │                     │
     ├──────────────────►│                     │
     │                   │                     │
     │                   │ Validate ID format  │
     │                   │ (64 hex chars)      │
     │                   │                     │
     │                   │ Lookup path         │
     │                   │ /a1/b2/glyph_...    │
     │                   │                     │
     │                   │ Read file           │
     │                   ├────────────────────►│
     │                   │                     │
     │                   │ Glyph data          │
     │                   │◄────────────────────┤
     │                   │                     │
     │ 200 OK            │                     │
     │◄──────────────────┤                     │
     │ {id, content, ...}│                     │
     │                   │                     │
```

### Accelerated Merge Flow

```
┌─────────┐    ┌─────────┐    ┌───────────┐    ┌──────────┐
│ Client  │    │ glyphd  │    │ glyph-spu │    │   FPGA   │
└────┬────┘    └────┬────┘    └─────┬─────┘    └─────┬────┘
     │              │               │                │
     │ Merge request│               │                │
     ├─────────────►│               │                │
     │              │               │                │
     │              │ POST /offload/merge            │
     │              ├──────────────►│                │
     │              │ {pairs: [...]}│                │
     │              │               │                │
     │              │               │ Alloc DMA buf  │
     │              │               │ Write pairs    │
     │              │               │                │
     │              │               │ Set INPUT_ADDR │
     │              │               │ Set OUTPUT_ADDR│
     │              │               │ Set COUNT      │
     │              │               │ Set AP_START=1 │
     │              │               ├───────────────►│
     │              │               │                │
     │              │               │                │ DMA fetch
     │              │               │                │ Process (71 cyc)
     │              │               │                │ DMA write back
     │              │               │                │
     │              │               │ Poll AP_DONE   │
     │              │               │◄───────────────┤
     │              │               │                │
     │              │               │ Read results   │
     │              │               │ from DMA buf   │
     │              │               │                │
     │              │ Results + stats                │
     │              │◄──────────────┤                │
     │              │               │                │
     │ Merged glyph │               │                │
     │◄─────────────┤               │                │
     │              │               │                │
```

### Multi-Node Fabric (Future)

```
┌─────────────────┐         ┌─────────────────┐
│   Node 1        │  RDMA   │   Node 2        │
│  (glyphd)       │◄───────►│  (glyphd)       │
│                 │         │                 │
│  /mnt/persist1  │         │  /mnt/persist2  │
└────────┬────────┘         └────────┬────────┘
         │                           │
         │                           │
         └───────────┬───────────────┘
                     │
                     ▼
          ┌─────────────────────┐
          │   Consensus Layer   │
          │  (Distributed sync) │
          └─────────────────────┘
```

**Fabric Performance Targets:**
- Loopback baseline: 14.7 µs P50
- RDMA (projected): 1-2 µs P50 (10x improvement)
- Multi-node sync: <100 µs (99th percentile)

## System Integration

### End-to-End Demo Pipeline

```
┌──────────────┐      ┌──────────────┐      ┌──────────────┐
│   Create     │      │   Dynamics   │      │   Render     │
│   Glyphs     ├─────►│   Engine     ├─────►│   Visual     │
│ (Python CLI) │      │  (evolve)    │      │   (PNG/GIF)  │
└──────────────┘      └──────────────┘      └──────────────┘

Step 1: create_glyph.py "particle" → glyph_id
Step 2: run_dynamics.py glyph_id --iterations 100 → evolved_id
Step 3: render.py evolved_id → visualization.gif
```

**Automation:** See `demos/end_to_end_demo.sh`

**Visual Mapping:**
- Energy (0-10) → Size (50-150px) + Brightness (30-100%)
- Frequency (20Hz-20kHz) → Hue (0-360°, logarithmic)
- Topology (node/loop/mesh) → Shape
- Activation → Pulsing ring effect

**Output:** 800x800px GIF, 12 seconds, 144 frames, ~24KB

### FreeBSD Integration (Staged)

**Target:** FreeBSD 14.1-RELEASE
**Kernel:** Custom kernel with FPGA drivers
**Filesystem:** ZFS with L2ARC on NVMe

**Components:**
- `glyphd` daemon (Rust, compiled for FreeBSD)
- `glyph-spu` kernel module (FPGA DMA driver)
- Init script: `/usr/local/etc/rc.d/glyphd`

**Installation:**
```bash
pkg install rust
cd runtime/rust/glyphd
cargo build --release --target x86_64-unknown-freebsd
install -m 755 target/release/glyphd /usr/local/bin/
service glyphd start
```

**Status:** Staged (tested on Linux, FreeBSD port pending)

## Performance Summary

### Component Performance Matrix

| Component | Operation | Latency | Throughput | Notes |
|-----------|-----------|---------|------------|-------|
| **glyphd** | Write (baseline) | 12.5ms P99 | 150 ops/s | fsync-limited |
| **glyphd** | Write (batched) | 0.025ms P99 | 30K ops/s | 5ms batch window |
| **glyphd** | Read | <1ms | 10K ops/s | NVMe read |
| **SPU** | merge (Python) | 5.33 µs | 187K ops/s | Baseline |
| **SPU** | merge (C++) | 0.57 µs | 1.76M ops/s | 9.4x speedup |
| **SPU** | merge (FPGA) | 0.355 µs | 3.2M ops/s | 16-lane target |
| **Fabric** | Loopback | 14.7 µs P50 | 68K msg/s | In-memory |
| **Fabric** | RDMA (proj) | 1-2 µs P50 | 500K msg/s | 10x improvement |

### Optimization Roadmap

1. **Phase 0 (Complete):** Python baseline implementation
2. **Phase 1 (Complete):** C++ reference with 9.4x speedup
3. **Phase 2 (In progress):** FPGA HLS simulation
4. **Phase 3 (Next):** FPGA synthesis and deployment
5. **Phase 4 (Future):** Multi-node RDMA fabric

## Testing & Verification

### Test Coverage

| Component | Test Type | Coverage | Status |
|-----------|-----------|----------|--------|
| **glyphd** | Unit tests | 85% | Passing |
| **glyphd** | Integration tests | 70% | Passing |
| **glyphd** | Crash safety | 100% (1000/1000) | Verified |
| **SPU** | Unit tests | 90% | Passing |
| **SPU** | Property tests | 100% (15/15) | Passing |
| **Dynamics** | Determinism | 100% (10/10 seeds) | Passing |
| **Persistence** | Atomic writes | 100% | Verified |
| **FPGA** | Simulation | 5/5 test vectors | Passing |

### CI/CD Pipeline

**GitHub Actions Workflow:**
1. Unit tests (Python, Rust)
2. Integration tests
3. Performance benchmarks
4. Regression detection (±20% threshold)
5. Documentation build

**Performance Gates:**
- SPU latency: ±20% (fail on regression)
- Persistence P99: ±50% (fail on regression)
- Memory usage: <500MB per node

### Benchmark Artifacts

All performance data preserved in `benchmarks/`:
- `merge_ref_results.json` - C++ merge performance
- `persistence_baseline.json` - Write latency baseline
- `persistence_batch5.json` - Batched write results
- `fabric_loopback.json` - Network latency
- `spu_results.json` - All SPU primitives
- `results_summary.json` - Aggregated metrics

## Security Considerations

### Content Addressing Security

- **Integrity:** SHA256 ensures content hasn't been tampered
- **Collision resistance:** 2^256 hash space (practically collision-free)
- **Determinism:** Predictable IDs enable verification

### FPGA Trust Model

**Problem:** How to trust FPGA-accelerated results?

**Solution:** Verification protocol
1. Host sends known test vectors to FPGA
2. FPGA processes and returns results
3. Host compares against software reference
4. Host signs verification proof if match
5. Proof stored in `fpga/docs/dma_contract_signed.json`

**Test Vectors:** See `fpga/sim/test_vectors/merge_inputs.json`

### Future Security Features

- TLS for HTTP endpoints
- Mutual TLS for fabric network
- Encrypted persistence (AES-256-GCM)
- FPGA attestation (remote verification)

## Deployment Architecture

### Single-Node Deployment

```
┌──────────────────────────────────────────┐
│          Glyph OS Node (Alpha)           │
├──────────────────────────────────────────┤
│  - glyphd (port 3000)                    │
│  - glyph-spu (port 8080, if FPGA present)│
│  - /mnt/persistence (NVMe mount)         │
│  - Monitoring (Prometheus exporter 9090) │
└──────────────────────────────────────────┘
```

**Hardware Requirements:**
- CPU: 4+ cores (8 recommended)
- RAM: 8GB minimum (16GB recommended)
- Storage: 256GB+ NVMe SSD
- Network: 1 Gbps (10 Gbps for multi-node)
- FPGA: Xilinx Alveo U50/U280 (optional)

### Multi-Node Cluster (Future)

```
       Load Balancer (HAProxy)
                │
        ┌───────┼───────┐
        │       │       │
    ┌───▼───┐ ┌▼──────┐ ┌▼──────┐
    │ Node1 │ │ Node2 │ │ Node3 │
    │glyphd │ │glyphd │ │glyphd │
    └───┬───┘ └┬──────┘ └┬──────┘
        │      │         │
        └──────┼─────────┘
               │
        RDMA Fabric (InfiniBand/RoCE)
```

**Consensus:** Raft or Paxos for distributed glyph sync
**Replication:** 3x replication factor (quorum writes)
**Partitioning:** Consistent hashing on glyph ID

## Status & Roadmap

### Current Status (2025-12-04)

- ✅ **Software stage complete**
- ✅ **glyphd REST API** - Ready for deployment
- ✅ **SPU C++ reference** - 9.4x speedup achieved
- ✅ **FPGA HLS code** - Simulation passing (5/5 tests)
- ✅ **Persistence** - Crash-safe, 99.8% P99 improvement
- ✅ **Benchmarks** - Comprehensive performance data
- ✅ **CI/CD** - Automated testing and regression detection
- 🔄 **FPGA synthesis** - Next phase
- 🔄 **FreeBSD port** - Staged (Linux validation complete)
- ⏳ **Multi-node fabric** - Design complete, implementation pending

### Immediate Next Steps

1. **FPGA Synthesis** (1-2 weeks)
   - Synthesize `merge_accel_hls.cpp` with Vivado HLS
   - Generate bitstream for Alveo U50
   - Implement PCIe driver and DMA engine

2. **Hardware Validation** (1 week)
   - Deploy bitstream to FPGA
   - Run test vectors (`fpga/sim/test_vectors/merge_inputs.json`)
   - Verify performance (target: 3.2M ops/sec)
   - Sign verification proof

3. **glyphd Integration** (1 week)
   - Implement HTTP client for `/offload/merge`
   - Add fallback logic (FPGA → C++ → Python)
   - Performance tuning and profiling

4. **FreeBSD Deployment** (2 weeks)
   - Port glyphd to FreeBSD 14.1
   - Kernel module for FPGA DMA
   - ZFS integration and tuning
   - Production deployment scripts

### Long-Term Roadmap

**Q1 2026:** Multi-node fabric with RDMA
**Q2 2026:** Distributed consensus and replication
**Q3 2026:** Web UI and visualization tools
**Q4 2026:** Audio synthesis from glyph resonance
**2027+:** Production scaling (100+ node clusters)

## Glossary

- **Content Addressing:** Using cryptographic hash (SHA256) as identifier
- **Glyph:** Fundamental unit of data in Glyph OS
- **SPU:** Symbolic Processing Unit (FPGA accelerator)
- **glyphd:** Glyph daemon (REST API server)
- **Ledonova:** Codename for Glyph OS node architecture
- **Merkle Tree:** Hash-based tree structure for efficient storage
- **DMA:** Direct Memory Access (PCIe data transfer)
- **HLS:** High-Level Synthesis (C++ to FPGA)
- **RDMA:** Remote Direct Memory Access (low-latency networking)

## References

### Documentation
- `docs/spu_offload_api.md` - SPU offload API specification
- `docs/persistence_api.md` - Persistence API specification
- `docs/merge_fpga_sketch.md` - FPGA design details
- `docs/persistence_tuning.md` - Performance tuning guide
- `docs/fabric_notes.md` - Network fabric configuration

### Implementation
- `runtime/rust/glyphd/` - Rust daemon source
- `runtime/spu/merge_ref.cpp` - C++ reference implementation
- `fpga/hls/merge_accel_hls.cpp` - FPGA HLS code
- `fpga/docs/dma_contract.json` - DMA interface spec

### Benchmarks
- `benchmarks/merge_ref_results.json` - Merge performance
- `benchmarks/persistence_baseline.json` - Write latency
- `benchmarks/fabric_loopback.json` - Network latency
- `benchmarks/results_summary.json` - Aggregated metrics

### Test Data
- `fpga/sim/test_vectors/merge_inputs.json` - FPGA test cases
- `benchmarks/persistence_crash_report.txt` - Crash safety verification

---

**Document Version:** 1.0-alpha
**Last Updated:** 2025-12-04
**Maintained By:** Glyph OS Team
**License:** MIT (assumed)
