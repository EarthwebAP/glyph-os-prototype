# SPU Merge Reference Implementation

C++ reference implementation of the glyph merge primitive for performance benchmarking and FPGA synthesis reference.

## Files

- **merge_ref.h** - Header with Glyph struct and merge() interface
- **merge_ref.cpp** - Implementation with built-in microbenchmark
- **merge_ref** - Compiled binary

## Building

```bash
g++ -O3 -std=c++17 merge_ref.cpp -o merge_ref
```

## Running

```bash
./merge_ref --iterations 100000 --out benchmarks/merge_ref_results.json
```

## Performance

Latest results (100K iterations):

```
Mean latency: 0.566 µs
Throughput: 1,765,217 ops/sec
Speedup vs Python: 9.4x
```

### Latency Distribution

- Min: 481 ns
- Median: 578 ns
- P95: 579 ns
- P99: 579 ns
- Max: 68,036 ns

### Hotspots (from profiling)

- SHA256 hash: ~85% of execution time
- Memory operations (memcpy): ~6%
- Energy comparison: ~2%
- Other: ~7%

## FPGA Integration

See [docs/merge_fpga_sketch.md](../../docs/merge_fpga_sketch.md) for:
- HLS/RTL implementation sketch
- DMA descriptor format
- Control register interface
- Performance projections

### Projected FPGA Performance

- **Single lane @ 200 MHz:** 350 ns latency, 200K ops/sec
- **16 lanes @ 200 MHz:** 350 ns latency, 3.2M ops/sec
- **Resource usage (16 lanes):** 3.5% LUTs, 2.3% FFs on Alveo U280
- **Speedup vs Python:** 17x (with 16 lanes)

## Optimization Notes

1. **SHA256 dominates (85% of time)**
   - Hardware IP core could reduce by 50-70%
   - Simplified non-crypto hash could reduce by 90%

2. **Memory operations (6%)**
   - SIMD vectorization could improve by 30%
   - Compiler already optimizes memcpy well

3. **Energy comparison (2%)**
   - Already minimal, single FP comparison

## Flamegraph

See `benchmarks/merge_ref_flame.svg` for visual hotspot analysis.

To generate a detailed flamegraph with perf (Linux only):

```bash
perf record -F 99 -g ./merge_ref --iterations 100000
perf script | ./tools/stackcollapse-perf.pl > out.folded
./tools/flamegraph.pl out.folded > merge_ref_flame.svg
```

## Integration with Python

See future work for Python bindings (pybind11):

```python
from spu import merge_ref

result = merge_ref.merge(glyph1, glyph2)
```

## References

- Python baseline: `benchmarks/bench_spu.py`
- FPGA documentation: `docs/merge_fpga_sketch.md`
- Benchmark results: `benchmarks/merge_ref_results.json`
