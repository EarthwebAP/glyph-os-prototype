# GlyphOS Fuzzing Guide

## Overview

This directory contains fuzzing harnesses for GlyphOS components. Fuzzing helps discover bugs, crashes, and security vulnerabilities by feeding randomized/mutated inputs to the parser.

## Supported Fuzzers

- **libFuzzer** (LLVM) - Recommended for continuous fuzzing
- **AFL** (American Fuzzy Lop) - Alternative coverage-guided fuzzer

## Building

### libFuzzer (Recommended)

```bash
# Build with libFuzzer and AddressSanitizer
clang -fsanitize=fuzzer,address -g -O1 \
  ci/fuzz_gdf.c -o bin/fuzz_gdf -lm

# Build with all sanitizers
clang -fsanitize=fuzzer,address,undefined,integer -g -O1 \
  ci/fuzz_gdf.c -o bin/fuzz_gdf_all -lm
```

### AFL

```bash
# Install AFL
pkg install afl  # FreeBSD
# or
sudo apt-get install afl  # Linux

# Build with AFL compiler
afl-clang-fast -fsanitize=address -g -O1 \
  ci/fuzz_gdf.c -o bin/fuzz_gdf_afl -lm
```

## Running

### libFuzzer

```bash
# Create corpus directory
mkdir -p corpus

# Seed corpus with valid GDF files
cp vault/*.gdf corpus/

# Run fuzzer (1 hour)
./bin/fuzz_gdf corpus/ -max_total_time=3600

# Run fuzzer (indefinitely)
./bin/fuzz_gdf corpus/ -jobs=4 -workers=4

# Run with specific options
./bin/fuzz_gdf corpus/ \
  -max_total_time=3600 \
  -max_len=10240 \
  -timeout=10 \
  -rss_limit_mb=2048
```

**libFuzzer Options**:
- `-max_total_time=N` - Run for N seconds
- `-max_len=N` - Maximum input length
- `-timeout=N` - Timeout per input (seconds)
- `-jobs=N` - Number of parallel jobs
- `-workers=N` - Number of worker processes
- `-dict=file` - Use dictionary file for mutations

### AFL

```bash
# Create input/output directories
mkdir -p afl_in afl_out

# Seed input directory
cp vault/*.gdf afl_in/

# Run AFL
afl-fuzz -i afl_in -o afl_out ./bin/fuzz_gdf_afl @@

# Run multiple AFL instances (master + slaves)
afl-fuzz -i afl_in -o afl_out -M fuzzer01 ./bin/fuzz_gdf_afl @@
afl-fuzz -i afl_in -o afl_out -S fuzzer02 ./bin/fuzz_gdf_afl @@
afl-fuzz -i afl_in -o afl_out -S fuzzer03 ./bin/fuzz_gdf_afl @@
```

## Corpus Management

### Initial Corpus

Create diverse valid inputs:

```bash
# Valid GDF files
cp vault/*.gdf corpus/

# Edge cases
echo "glyph_id: test" > corpus/minimal.gdf
echo "glyph_id: $(python3 -c 'print("A"*1000)')" > corpus/long_id.gdf
echo "resonance_freq: -1" > corpus/negative.gdf
echo "resonance_freq: 999999" > corpus/huge.gdf

# Invalid/malformed inputs
echo "{{{{" > corpus/malformed1.gdf
echo "glyph_id" > corpus/malformed2.gdf
printf "\x00\x00\x00\x00" > corpus/null_bytes.gdf
```

### Corpus Minimization

```bash
# Minimize corpus (libFuzzer)
./bin/fuzz_gdf -merge=1 corpus_min corpus/

# Minimize corpus (AFL)
afl-cmin -i afl_out/queue -o afl_min -- ./bin/fuzz_gdf_afl @@
afl-tmin -i afl_out/queue/id:000001 -o minimized.gdf -- ./bin/fuzz_gdf_afl @@
```

## Analyzing Results

### Crashes

```bash
# Reproduce crash (libFuzzer)
./bin/fuzz_gdf crash-<hash>

# Reproduce crash (AFL)
./bin/fuzz_gdf_afl afl_out/crashes/id:000000,sig:06,*

# Debug with GDB
gdb --args ./bin/fuzz_gdf crash-<hash>
```

### Coverage

```bash
# Generate coverage report (libFuzzer)
clang -fprofile-instr-generate -fcoverage-mapping \
  ci/fuzz_gdf.c -o bin/fuzz_gdf_cov -lm

LLVM_PROFILE_FILE="fuzz.profraw" ./bin/fuzz_gdf_cov corpus/ -runs=100000
llvm-profdata merge -sparse fuzz.profraw -o fuzz.profdata
llvm-cov show bin/fuzz_gdf_cov -instr-profile=fuzz.profdata > coverage.txt
llvm-cov report bin/fuzz_gdf_cov -instr-profile=fuzz.profdata

# AFL coverage
afl-showmap -o /dev/null -i corpus/ -- ./bin/fuzz_gdf_afl @@
```

## Dictionary Files

Create `gdf.dict` to guide mutations:

```
# GDF keywords
kw1="glyph_id"
kw2="chronocode"
kw3="parent_glyphs"
kw4="resonance_freq"
kw5="field_magnitude"
kw6="coherence"
kw7="activation_simulation"
kw8="entangle"
kw9="resonate"
kw10="amplify"
kw11="decay"
kw12="phase_shift"
kw13="stabilize"

# Delimiters
delim1=":"
delim2="|"
delim3=","
delim4="("
delim5=")"

# Common values
val1="000"
val2="440.0"
val3="100"
```

Use dictionary:
```bash
./bin/fuzz_gdf corpus/ -dict=gdf.dict
```

## Continuous Fuzzing

### Cron Job

```cron
# Fuzz for 6 hours every night
0 2 * * * cd /usr/src/glyphos && ./bin/fuzz_gdf corpus/ -max_total_time=21600 >> logs/fuzz.log 2>&1
```

### tmux Session

```bash
# Start persistent fuzzing session
tmux new -s glyphos-fuzz -d
tmux send-keys -t glyphos-fuzz "cd /usr/src/glyphos" C-m
tmux send-keys -t glyphos-fuzz "./bin/fuzz_gdf corpus/ -jobs=8 -workers=8" C-m

# Attach to session
tmux attach -t glyphos-fuzz

# Detach: Ctrl-b d
```

## Best Practices

1. **Run fuzzing for at least 7 days** before release
2. **Use sanitizers** (ASan, UBSan) to detect memory errors
3. **Minimize corpus** regularly to reduce test case redundancy
4. **Seed corpus** with valid inputs and edge cases
5. **Monitor coverage** to ensure all code paths are tested
6. **Triage crashes** immediately and file bug reports
7. **Re-test after fixes** with crash-inducing inputs

## Integration with CI

See `.github/workflows/ci.yml` for automated fuzzing in CI pipeline.

## Troubleshooting

### "Couldn't mmap" Error

```bash
# Increase shared memory limit (FreeBSD)
sysctl kern.ipc.shmmax=67108864
sysctl kern.ipc.shmall=32768

# Or use smaller corpus
./bin/fuzz_gdf corpus/ -rss_limit_mb=1024
```

### Out of Memory

```bash
# Limit memory usage
./bin/fuzz_gdf corpus/ -rss_limit_mb=2048 -malloc_limit_mb=2048
```

### Slow Performance

```bash
# Use more cores
./bin/fuzz_gdf corpus/ -jobs=$(nproc) -workers=$(nproc)

# Reduce timeout
./bin/fuzz_gdf corpus/ -timeout=5
```

## References

- libFuzzer: https://llvm.org/docs/LibFuzzer.html
- AFL: https://github.com/google/AFL
- Fuzzing Book: https://www.fuzzingbook.org/
