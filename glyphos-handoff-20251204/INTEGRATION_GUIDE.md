# GlyphOS Phases 1-4 Integration Guide

**Version**: 1.0.0
**Last Updated**: 2025-12-05
**Target System**: FreeBSD `/usr/src/glyphos/`
**Status**: All 4 Phases Complete and Ready for Integration

---

## Table of Contents

1. [Quick Start Commands](#quick-start-commands)
2. [File Placement Table](#file-placement-table)
3. [Expected Test Outputs](#expected-test-outputs)
4. [Verification Checklist](#verification-checklist)
5. [Troubleshooting](#troubleshooting)
6. [Next Steps](#next-steps)

---

## Quick Start Commands

### One-Command Setup (Copy & Paste Ready)

```bash
# Step 1: Create GlyphOS directory structure
sudo mkdir -p /usr/src/glyphos/{src,bin,scripts,logs,data}
sudo chown -R $(whoami) /usr/src/glyphos

# Step 2: Extract all phase files from /tmp/
cp /tmp/phase1_unified_pipeline.sh /usr/src/glyphos/scripts/unified_pipeline.sh
cp /tmp/phase2_cse_core.c /usr/src/glyphos/src/cse_core.c
cp /tmp/phase3_substrate_core.c /usr/src/glyphos/src/substrate_core.c
cp /tmp/phase4_glyph_interpreter.c /usr/src/glyphos/src/glyph_interpreter.c

# Step 3: Make scripts executable
chmod +x /usr/src/glyphos/scripts/unified_pipeline.sh

# Step 4: Run unified pipeline (builds all modules + runs tests)
cd /usr/src/glyphos/scripts
./unified_pipeline.sh --clean --verbose

# Step 5: Verify all 6 modules compiled and passed tests
cat /usr/src/glyphos/logs/status_report_*.txt
```

### Quick Verification (After Setup)

```bash
# Test all binaries are executable and functional
ls -lh /usr/src/glyphos/bin/

# Expected output (6 executables):
# glyphos_main
# cse_runtime
# substrate_layer
# cse_core
# substrate_core
# glyph_interp

# Run individual tests
/usr/src/glyphos/bin/cse_core --test
/usr/src/glyphos/bin/substrate_core --test
/usr/src/glyphos/bin/glyph_interp --test
```

---

## File Placement Table

| Phase | Source File | Destination | Compiler Command | Test Command |
|-------|-------------|-------------|------------------|--------------|
| **1** | `phase1_unified_pipeline.sh` | `/usr/src/glyphos/scripts/unified_pipeline.sh` | N/A (shell script) | `./unified_pipeline.sh --clean --verbose` |
| **2** | `phase2_cse_core.c` | `/usr/src/glyphos/src/cse_core.c` | `cc -o /usr/src/glyphos/bin/cse_core /usr/src/glyphos/src/cse_core.c` | `/usr/src/glyphos/bin/cse_core --test` |
| **3** | `phase3_substrate_core.c` | `/usr/src/glyphos/src/substrate_core.c` | `cc -o /usr/src/glyphos/bin/substrate_core /usr/src/glyphos/src/substrate_core.c -lm` | `/usr/src/glyphos/bin/substrate_core --test` |
| **4** | `phase4_glyph_interpreter.c` | `/usr/src/glyphos/src/glyph_interpreter.c` | `cc -o /usr/src/glyphos/bin/glyph_interp /usr/src/glyphos/src/glyph_interpreter.c` | `/usr/src/glyphos/bin/glyph_interp --test` |

### Detailed Compilation Commands

#### Phase 2: CSE Core (477 lines)

```bash
# Basic compilation
cc -o /usr/src/glyphos/bin/cse_core /usr/src/glyphos/src/cse_core.c

# With debug symbols (recommended for development)
cc -g -o /usr/src/glyphos/bin/cse_core /usr/src/glyphos/src/cse_core.c

# With optimizations (production)
cc -O2 -o /usr/src/glyphos/bin/cse_core /usr/src/glyphos/src/cse_core.c
```

**Dependencies**: `stdio.h`, `stdlib.h`, `string.h`, `stdint.h` (all standard C library)

**Size**: ~477 lines | **Type**: Cognitive Symbolic Engine core runtime

---

#### Phase 3: Substrate Core (Full implementation in /tmp)

```bash
# Requires math library (-lm flag)
cc -o /usr/src/glyphos/bin/substrate_core /usr/src/glyphos/src/substrate_core.c -lm

# With debug symbols and math library
cc -g -lm -o /usr/src/glyphos/bin/substrate_core /usr/src/glyphos/src/substrate_core.c

# With optimizations
cc -O2 -lm -o /usr/src/glyphos/bin/substrate_core /usr/src/glyphos/src/substrate_core.c
```

**Dependencies**: `stdio.h`, `stdlib.h`, `string.h`, `stdint.h`, `math.h`, `-lm` (libm)

**Size**: 600+ lines (full implementation) | **Type**: Quantum substrate field layer

---

#### Phase 4: Glyph Interpreter (269 lines)

```bash
# Basic compilation
cc -o /usr/src/glyphos/bin/glyph_interp /usr/src/glyphos/src/glyph_interpreter.c

# With debug symbols
cc -g -o /usr/src/glyphos/bin/glyph_interp /usr/src/glyphos/src/glyph_interpreter.c

# With optimizations (production)
cc -O2 -o /usr/src/glyphos/bin/glyph_interp /usr/src/glyphos/src/glyph_interpreter.c
```

**Dependencies**: `stdio.h`, `stdlib.h`, `string.h`, `stdint.h` (all standard C library)

**Size**: ~269 lines | **Type**: GDF parser and glyph activation simulator

---

## Expected Test Outputs

Each module has a built-in `--test` mode. Here's what to expect:

### Phase 2: CSE Core Test Output

```
CSE Core Test Mode
==================

Test 1: Basic arithmetic
  Result: 30 (expected: 30)
  Status: PASS

Test 2: Field state resonance
  Magnitude: 2.00 (expected: 2.00)
  Phase: 0.50
  Coherence: 100
  Status: PASS

CSE Core operational.
```

**Success Criteria**:
- Both tests show "PASS"
- Arithmetic result equals 30 (10 + 20)
- Magnitude is within ±0.01 of 2.00
- Exit code is 0

---

### Phase 3: Substrate Core Test Output

```
Substrate Core Test Mode
========================

Test 1: Field initialization
  Magnitude: 1.00
  Phase: 0.00
  Coherence: 100
  Status: PASS

Test 2: Field modulation
  New magnitude: [varies with resonance]
  New phase: [incremental]
  Coherence decay: [100 → lower]
  Status: PASS

Substrate Core operational.
```

**Success Criteria**:
- Both tests show "PASS"
- Field magnitude is positive number
- Phase advances with each step
- Coherence decreases or stays stable
- Exit code is 0

---

### Phase 4: Glyph Interpreter Test Output

```
Glyph Interpreter Test Mode
============================

Test 1: GDF parsing
  Glyph ID: 001
  Chronocode: 20250101_120000
  Parents: 1
  Resonance: 440.0 Hz
  Status: PASS

Test 2: Activation simulation
  [ACTIVATE] Glyph 001 | Freq: 440.0 Hz | Mag: 1.00 | Coh: 100
  [STEP 1] Mag: [varies] | Phase: 0.10 | Coh: 99
  [STEP 2] Mag: [varies] | Phase: 0.20 | Coh: 98
  [STEP 3] Mag: [varies] | Phase: 0.30 | Coh: 97
  Status: PASS

Test 3: Inheritance chain
  [INHERIT] Level 0: Glyph 001
  [INHERIT] Parent: 000
  [INHERIT] Total inherited resonance: 1.00
  Status: PASS

Glyph Interpreter operational.
```

**Success Criteria**:
- All three tests show "PASS"
- Glyph ID parses correctly
- Activation trace shows 3+ steps
- Phase advances (0.10, 0.20, 0.30)
- Coherence decreases with each step
- Exit code is 0

---

### Phase 1: Unified Pipeline Test Output

```
=========================================
  GlyphOS Unified Build Pipeline
  [timestamp]
=========================================

[→] Step 0: Environment verification
[✓] GlyphOS root: /usr/src/glyphos
[✓] Build log: /usr/src/glyphos/logs/build_*.log

[→] Step 1: Clean build environment
[✓] Build artifacts cleaned

[→] Step 2: Building all modules
[✓] Building: main.c → glyphos_main
[✓]   ✓ glyphos_main
[✓] Building: cse_test.c → cse_runtime
[✓]   ✓ cse_runtime
[✓] Building: substrate_test.c → substrate_layer
[✓]   ✓ substrate_layer
[✓] Building: cse_core.c → cse_core
[✓]   ✓ cse_core
[✓] Building: substrate_core.c → substrate_core
[✓]   ✓ substrate_core
[✓] Building: glyph_interpreter.c → glyph_interp
[✓]   ✓ glyph_interp
[✓] Build summary: 6/6 succeeded, 0 failed

[→] Step 3: Running smoke tests
[✓] Running: smoke_glyphos.sh
[✓]   ✓ Toolchain: OK
[✓] Running: smoke_cse.sh
[✓]   ✓ CSE Runtime: OK
[✓] Running: smoke_substrate.sh
[✓]   ✓ Substrate: OK
[✓] Running: cse_core --test
[✓]   ✓ CSE Core operational
[✓] Running: substrate_core --test
[✓]   ✓ Substrate Core operational
[✓] Running: glyph_interp --test
[✓]   ✓ Glyph Interpreter operational
[✓] Test summary: 6/6 passed, 0 failed

[→] Step 4: Generating unified status report
[✓] Report saved: /usr/src/glyphos/logs/status_report_*.txt

=========================================
✓ PIPELINE SUCCESS
=========================================
```

**Status Report Shows**:
```
BUILD PHASE
-----------
Total modules:    6
Built:            6
Failed:           0
Status:           PASS

TEST PHASE
----------
Total tests:      6
Passed:           6
Failed:           0
Status:           PASS

COMPONENT STATUS
----------------
✓ Toolchain
✓ CSE Runtime
✓ Substrate Layer
✓ CSE Core
✓ Substrate Core
✓ Glyph Interpreter

CI READY
--------
Exit code: 0 (PASS)
CI mode:   DISABLED
```

**Success Criteria**:
- All 6 modules built successfully
- All 6 tests passed
- Exit code is 0
- Report shows "6/6 PASS" in multiple places

---

## Verification Checklist

Use this checklist to verify complete integration:

### Pre-Integration

- [ ] Working directory: `/home/daveswo/glyph-os-prototype/glyphos-handoff-20251204/`
- [ ] All 4 phase files exist in `/tmp/`:
  - [ ] `/tmp/phase1_unified_pipeline.sh` (369 lines)
  - [ ] `/tmp/phase2_cse_core.c` (477 lines)
  - [ ] `/tmp/phase3_substrate_core.c` (600+ lines)
  - [ ] `/tmp/phase4_glyph_interpreter.c` (269 lines)
- [ ] FreeBSD system or WSL with gcc/clang available
- [ ] `/usr/src/glyphos/` directory writable (or can use sudo)

### File Placement

- [ ] `unified_pipeline.sh` copied to `/usr/src/glyphos/scripts/`
- [ ] `cse_core.c` copied to `/usr/src/glyphos/src/`
- [ ] `substrate_core.c` copied to `/usr/src/glyphos/src/`
- [ ] `glyph_interpreter.c` copied to `/usr/src/glyphos/src/`
- [ ] Script is executable: `test -x /usr/src/glyphos/scripts/unified_pipeline.sh`

### Compilation

- [ ] `cc` or `gcc` compiler available: `which cc` or `which gcc`
- [ ] Standard C library headers present: `echo '#include <stdio.h>' | cc -c -x c -`
- [ ] Math library available: `cc -print-file-name=libm.a` shows path
- [ ] CSE Core compiles without errors
- [ ] Substrate Core compiles without errors (with `-lm` flag)
- [ ] Glyph Interpreter compiles without errors
- [ ] All binaries are executable and in `/usr/src/glyphos/bin/`

### Tests Pass

- [ ] **CSE Core test**: `cse_core --test` shows "CSE Core operational" with PASS
- [ ] **Substrate Core test**: `substrate_core --test` shows "operational" with PASS
- [ ] **Glyph Interpreter test**: `glyph_interp --test` shows "operational" with PASS
- [ ] **Unified Pipeline**: `unified_pipeline.sh --clean` shows "6/6 succeeded"

### Integration Complete

- [ ] Unified status report exists: `ls /usr/src/glyphos/logs/status_report_*.txt`
- [ ] Status report shows **6/6 PASS**
- [ ] All logs are in `/usr/src/glyphos/logs/`
- [ ] Binary directory has 6 executables: `ls -1 /usr/src/glyphos/bin/ | wc -l`
- [ ] No build errors in log: `! grep -i "error\|failed" /usr/src/glyphos/logs/build_*.log`

### Production Ready

- [ ] Binaries are optimized (optional): Recompile with `-O2`
- [ ] Binaries have debug symbols (optional): Compile with `-g`
- [ ] Logs are being written: `test -s /usr/src/glyphos/logs/build_*.log`
- [ ] Pipeline can be re-run: `unified_pipeline.sh --clean` works multiple times
- [ ] Next phase dependencies understood (see Next Steps)

---

## Troubleshooting

### Compilation Issues

#### Error: "cc: command not found"

**Solution**:
```bash
# On FreeBSD
pkg install gcc
# or
pkg install clang

# On Linux/WSL
sudo apt-get install build-essential
# or
sudo apt-get install gcc
```

---

#### Error: "math.h: No such file or directory" (Phase 3)

**Solution**:
```bash
# Make sure to use -lm flag when compiling substrate_core.c
cc -o /usr/src/glyphos/bin/substrate_core /usr/src/glyphos/src/substrate_core.c -lm

# On FreeBSD, math library is part of libc, but still needs -lm flag:
cc -lm -o /usr/src/glyphos/bin/substrate_core /usr/src/glyphos/src/substrate_core.c
```

---

#### Error: "undefined reference to `sqrt'" or similar math functions

**Solution**: You're missing the `-lm` flag for substrate_core.c compilation.

```bash
# WRONG
cc -o /usr/src/glyphos/bin/substrate_core /usr/src/glyphos/src/substrate_core.c

# CORRECT
cc -o /usr/src/glyphos/bin/substrate_core /usr/src/glyphos/src/substrate_core.c -lm
```

---

### Path Issues

#### Error: "GlyphOS root not found: /usr/src/glyphos"

**Solution**:
```bash
# Create the directory structure
sudo mkdir -p /usr/src/glyphos/{src,bin,scripts,logs,data}

# Change ownership if needed
sudo chown -R $(whoami) /usr/src/glyphos

# Verify
ls -la /usr/src/glyphos/
```

---

#### Error: "Source directory not found: /usr/src/glyphos/src"

**Solution**:
```bash
# Verify all phase files are in the correct locations
ls -la /usr/src/glyphos/src/cse_core.c
ls -la /usr/src/glyphos/src/substrate_core.c
ls -la /usr/src/glyphos/src/glyph_interpreter.c

# If missing, copy them:
cp /tmp/phase2_cse_core.c /usr/src/glyphos/src/cse_core.c
cp /tmp/phase3_substrate_core.c /usr/src/glyphos/src/substrate_core.c
cp /tmp/phase4_glyph_interpreter.c /usr/src/glyphos/src/glyph_interpreter.c
```

---

### Runtime Issues

#### Test Output Shows Warnings or Unexpected Values

**Expected behavior**: Tests may show floating-point precision differences. This is normal.

```bash
# This is OK:
Magnitude: 2.00000001 (expected: 2.00)

# Check if within acceptable tolerance:
if [ $(echo "2.00000001 >= 1.99 && 2.00000001 <= 2.01" | bc -l) -eq 1 ]; then
    echo "PASS"
fi
```

---

#### Pipeline Hangs or Takes Too Long

**Solution**:
```bash
# If pipeline hangs, you can interrupt with Ctrl+C
# Then check what failed:
tail -100 /usr/src/glyphos/logs/build_*.log

# Run with timeout
timeout 300 /usr/src/glyphos/scripts/unified_pipeline.sh --clean

# Or run individual tests
/usr/src/glyphos/bin/cse_core --test
```

---

#### Tests Pass Individually But Fail in Pipeline

**Solution**:
```bash
# Check CI mode requirements
# The unified pipeline uses set -e (exit on error)

# Run in verbose mode to see which command fails
/usr/src/glyphos/scripts/unified_pipeline.sh --verbose

# Check logs
cat /usr/src/glyphos/logs/build_*.log
cat /usr/src/glyphos/logs/test_*.log
```

---

### Permission Issues

#### Error: "Permission denied" when running unified_pipeline.sh

**Solution**:
```bash
# Make script executable
chmod +x /usr/src/glyphos/scripts/unified_pipeline.sh

# Run it
/usr/src/glyphos/scripts/unified_pipeline.sh

# If /usr/src/glyphos is root-owned, use sudo
sudo /usr/src/glyphos/scripts/unified_pipeline.sh
```

---

#### Binaries are Not Executable

**Solution**:
```bash
# Check permissions
ls -la /usr/src/glyphos/bin/

# Fix if needed
chmod +x /usr/src/glyphos/bin/*

# Verify
file /usr/src/glyphos/bin/cse_core
```

---

### Dependency Issues

#### Error: Missing smoke_*.sh scripts

**Normal**: The pipeline checks for these scripts but continues if they don't exist. This is expected for fresh installations.

```bash
# These are optional. If you want them, check /tmp or the original build directory
find /tmp -name "smoke*.sh" 2>/dev/null
find /home/daveswo -name "smoke*.sh" 2>/dev/null
```

---

#### Header Files Not Found

**Solution**:
```bash
# Install development headers
# On FreeBSD
pkg install gcc

# On Debian/Ubuntu
sudo apt-get install build-essential

# On RHEL/CentOS
sudo yum install gcc
```

---

### Status Report Issues

#### Can't Find Status Report

**Solution**:
```bash
# Status report is generated in logs directory
ls -la /usr/src/glyphos/logs/

# Find most recent report
ls -ltr /usr/src/glyphos/logs/status_report_*.txt | tail -1

# View it
cat /usr/src/glyphos/logs/status_report_$(date +%Y%m%d_*)
```

---

#### Status Report Shows Failed Tests

**Solution**:
```bash
# Check individual test logs
cat /usr/src/glyphos/logs/test_*.log

# Run failing test in verbose mode
strace /usr/src/glyphos/bin/cse_core --test 2>&1 | tail -50

# Check for core dumps
dmesg | grep -i segmentation
ulimit -a
```

---

## Next Steps

### Phase 5: Glyph Vault Integration

The unified pipeline is ready. Next phase is to integrate the Glyph Vault:

**What to do**:
1. Use `/usr/src/glyphos/bin/glyph_interp` to load and parse GDF files
2. Store parsed glyphs in a vault structure (SQLite or custom format)
3. Implement persistence layer hooks in CSE Core
4. Add vault loading to the startup sequence

**Expected file**: `phase5_glyph_vault.c` (~400 lines)

```c
// Vault integration will hook into CSE's substrate_read/write mechanisms
// See cse_core.c lines 110-113 for hook callbacks
CSEContext *ctx = cse_create_context();
ctx->substrate_context = vault_instance;
ctx->substrate_read_hook = vault_read_callback;
ctx->substrate_write_hook = vault_write_callback;
```

---

### GDF Schema Expansion

Current GDF v2.0 schema supports:
- glyph_id
- chronocode (temporal marker)
- parent_glyphs (inheritance chain)
- resonance_freq
- field_magnitude
- coherence
- material_spec
- activation_script
- observer_response
- contributor_id
- vault_index
- frequency_signature

**Future expansions**:
1. **Harmonic relationships**: Add parent frequency ratios
2. **Activation conditions**: Conditional script execution
3. **Observer tracking**: Record who activated each glyph
4. **Vault versioning**: Track GDF schema versions
5. **Quantum states**: Extended coherence tracking

---

### CI/CD Setup

Once all phases are integrated, set up CI/CD:

```bash
# Example GitHub Actions workflow
name: GlyphOS Build & Test
on: [push, pull_request]
jobs:
  build:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v2
      - name: Install dependencies
        run: sudo apt-get install -y build-essential
      - name: Build and test
        run: |
          mkdir -p /usr/src/glyphos/{src,bin,scripts,logs}
          cp -r . /usr/src/glyphos/
          /usr/src/glyphos/scripts/unified_pipeline.sh --ci
```

---

### Monitoring & Observability

Set up monitoring for production:

```bash
# Add health check endpoint to each binary
# Modify main() to accept --health flag

# Example:
/usr/src/glyphos/bin/glyph_interp --health
# Output: {"status": "ok", "version": "0.1.0", "uptime": 123}

# Prometheus metrics:
/usr/src/glyphos/bin/cse_core --metrics
# Output: cse_core_stack_usage_bytes 512
#         cse_core_instructions_executed_total 1024
```

---

### Documentation

Maintain documentation as new phases are added:

1. **INTEGRATION_GUIDE.md** (this file)
   - Update with each new phase
   - Add new test outputs
   - Update verification checklist

2. **API_DOCUMENTATION.md** (create)
   - Document each module's public API
   - Function signatures and parameters
   - Integration points

3. **ARCHITECTURE.md** (create)
   - System architecture overview
   - Data flow between phases
   - State management strategy

---

## Summary

### What You Now Have

✅ **Phase 1**: Unified build and test pipeline (370 lines)
✅ **Phase 2**: CSE Core runtime (477 lines)
✅ **Phase 3**: Substrate Core field layer (600+ lines)
✅ **Phase 4**: Glyph Interpreter with GDF parser (269 lines)

**Total**: 1,716+ lines of production-ready code

### What This Enables

1. **Symbolic Computation**: CSE Core can execute glyph programs
2. **Field Simulation**: Substrate layer models quantum-like fields
3. **Glyph Activation**: Interpreter loads GDF files and activates glyphs
4. **Automated Building**: Unified pipeline compiles and tests everything
5. **CI/CD Ready**: Pipeline supports `--ci` mode for automation

### Quick Status Check

```bash
# One command to verify everything is integrated:
/usr/src/glyphos/scripts/unified_pipeline.sh --clean && \
echo "✓ ALL PHASES INTEGRATED SUCCESSFULLY" || \
echo "✗ Integration failed - see logs"
```

---

## Additional Resources

### Inside /usr/src/glyphos/

- **Binaries**: `/usr/src/glyphos/bin/` - All compiled modules
- **Source**: `/usr/src/glyphos/src/` - All C source files
- **Scripts**: `/usr/src/glyphos/scripts/` - Build pipeline and utilities
- **Logs**: `/usr/src/glyphos/logs/` - Build and test logs
- **Data**: `/usr/src/glyphos/data/` - Runtime data and glyphs

### Commands Reference

```bash
# Build everything
/usr/src/glyphos/scripts/unified_pipeline.sh --clean

# Build incrementally (don't clean previous artifacts)
/usr/src/glyphos/scripts/unified_pipeline.sh

# Build with verbose output
/usr/src/glyphos/scripts/unified_pipeline.sh --verbose

# CI mode (exit with error if any test fails)
/usr/src/glyphos/scripts/unified_pipeline.sh --ci

# Test individual modules
/usr/src/glyphos/bin/cse_core --test
/usr/src/glyphos/bin/substrate_core --test
/usr/src/glyphos/bin/glyph_interp --test

# Load and test a GDF file
/usr/src/glyphos/bin/glyph_interp --load /path/to/glyph.gdf
```

---

## Support

For issues or questions:

1. **Check the logs**: `/usr/src/glyphos/logs/build_*.log`
2. **Run with verbose**: `unified_pipeline.sh --verbose`
3. **Test individually**: Run each module's `--test` mode
4. **Review troubleshooting**: See Troubleshooting section above

---

**Ready to integrate?** Start with the Quick Start Commands at the top of this guide.

---

*Generated for GlyphOS Integration 2025-12-05*
