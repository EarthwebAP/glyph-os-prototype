/**
 * SPU Merge Primitive - C++ Reference Implementation
 *
 * Hardware-friendly implementation of the glyph merge operation.
 * Optimized for parallelization and FPGA synthesis.
 */

#include <cstdint>
#include <cstring>
#include <algorithm>
#include <string>

// SHA256 implementation (minimal for hardware)
#include "sha256.h"

namespace spu {

// Glyph structure (fixed-size for hardware efficiency)
struct Glyph {
    char id[64];                // SHA256 hash as hex string
    char content[256];          // Fixed-size content buffer
    uint16_t content_len;       // Actual content length

    double energy;              // Energy level
    uint32_t activation_count;  // Activation counter
    uint64_t last_update_time;  // Last update timestamp

    // Merge provenance
    char parent1_id[64];
    char parent2_id[64];
};

/**
 * Merge two glyphs with energy-based precedence
 *
 * @param g1 First glyph
 * @param g2 Second glyph
 * @param result Output merged glyph
 *
 * Performance: O(n) where n = content length
 * Hardware: Fully pipelineable, SHA256 can run in parallel
 */
void merge(const Glyph& g1, const Glyph& g2, Glyph& result) {
    // Step 1: Determine precedence (1 comparison, 2 cycles)
    const Glyph* primary;
    const Glyph* secondary;

    if (g1.energy >= g2.energy) {
        primary = &g1;
        secondary = &g2;
    } else {
        primary = &g2;
        secondary = &g1;
    }

    // Step 2: Concatenate content (memcpy, pipelineable)
    // Format: "primary + secondary"
    uint16_t pos = 0;

    // Copy primary content
    memcpy(result.content + pos, primary->content, primary->content_len);
    pos += primary->content_len;

    // Add separator
    result.content[pos++] = ' ';
    result.content[pos++] = '+';
    result.content[pos++] = ' ';

    // Copy secondary content
    memcpy(result.content + pos, secondary->content, secondary->content_len);
    pos += secondary->content_len;

    result.content_len = pos;

    // Step 3: Compute ID via SHA256 hash (parallel with next steps)
    // In hardware, this runs in dedicated SHA256 unit
    sha256_hash(result.content, result.content_len, result.id);

    // Step 4: Sum energies (1 FP add, 1 cycle)
    result.energy = primary->energy + secondary->energy;

    // Step 5: Merge metadata (max operations, 2 cycles each)
    result.activation_count = std::max(primary->activation_count,
                                       secondary->activation_count);
    result.last_update_time = std::max(primary->last_update_time,
                                       secondary->last_update_time);

    // Step 6: Record provenance
    memcpy(result.parent1_id, primary->id, 64);
    memcpy(result.parent2_id, secondary->id, 64);
}

/**
 * Vectorized merge for batch processing
 *
 * @param pairs Array of glyph pairs to merge
 * @param results Output array of merged glyphs
 * @param count Number of pairs
 *
 * Hardware: Fully parallel, process N pairs simultaneously
 */
void merge_batch(const Glyph* pairs, Glyph* results, size_t count) {
    // Each merge is independent - perfect for parallelization
    #pragma omp parallel for
    for (size_t i = 0; i < count; i++) {
        merge(pairs[i*2], pairs[i*2 + 1], results[i]);
    }
}

/**
 * Performance characteristics:
 *
 * Latency breakdown (reference CPU):
 * - Energy comparison: ~1ns (1 cycle @ 1GHz)
 * - Content concatenation: ~10ns (depends on content size)
 * - SHA256 hash: ~100ns (can pipeline with specialized unit)
 * - Energy sum: ~1ns (1 FP cycle)
 * - Metadata merge: ~2ns (2 max ops)
 * Total: ~114ns (~5.3μs observed in Python, includes overhead)
 *
 * FPGA optimization potential:
 * - Dedicated SHA256 unit: 200MHz @ 64 cycles = 320ns
 * - Parallel memcpy units: ~20ns
 * - FP adder: ~5ns
 * - Total FPGA latency: ~350ns (15x speedup potential)
 *
 * Throughput (batch):
 * - CPU: ~187K ops/sec (observed)
 * - FPGA (16 parallel units): ~2.8M ops/sec (projected)
 */

} // namespace spu
