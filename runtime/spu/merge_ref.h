/**
 * SPU Merge Primitive - C++ Reference Header
 *
 * Hardware-friendly implementation of the glyph merge operation.
 * Optimized for CPU microbenchmarking and FPGA synthesis reference.
 */

#ifndef SPU_MERGE_REF_H
#define SPU_MERGE_REF_H

#include <cstdint>
#include <string>

namespace spu {

// Glyph structure (fixed-size for hardware efficiency)
struct Glyph {
    char id[65];                // SHA256 hash as hex string (64 + null)
    char content[256];          // Fixed-size content buffer
    uint16_t content_len;       // Actual content length

    double energy;              // Energy level
    uint32_t activation_count;  // Activation counter
    uint64_t last_update_time;  // Last update timestamp

    // Merge provenance
    char parent1_id[65];
    char parent2_id[65];

    // Constructor
    Glyph();
};

/**
 * Merge two glyphs with energy-based precedence
 *
 * @param g1 First glyph
 * @param g2 Second glyph
 * @param result Output merged glyph
 *
 * Performance: O(n) where n = content length
 * Latency: ~350ns (CPU), pipelineable for FPGA
 */
void merge(const Glyph& g1, const Glyph& g2, Glyph& result);

/**
 * Compute SHA256 hash of content
 *
 * @param data Input data buffer
 * @param len Length of data in bytes
 * @param output Output buffer (must be 65 bytes for hex string + null)
 */
void sha256_hash(const char* data, size_t len, char* output);

} // namespace spu

#endif // SPU_MERGE_REF_H
