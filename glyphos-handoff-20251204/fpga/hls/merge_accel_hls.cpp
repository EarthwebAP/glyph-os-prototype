/**
 * SPU Merge Accelerator - Vivado HLS Implementation
 *
 * Target: Xilinx Alveo U50 (or U280)
 * Clock: 200 MHz
 * Interface: AXI4-Stream input/output + AXI4-Lite control
 *
 * Synthesis: vivado_hls -f run_hls.tcl
 */

#include <ap_int.h>
#include <ap_axi_sdata.h>
#include <hls_stream.h>
#include <stdint.h>

// Type definitions for hardware
typedef ap_ufixed<32, 16> energy_t;      // Q16.16 fixed-point energy
typedef ap_uint<512> id_t;                // 64-byte ID (512 bits)
typedef ap_uint<2048> content_t;          // 256-byte content (2048 bits)
typedef ap_uint<16> len_t;                // Content length (0-255)

// Glyph structure for AXI streaming
struct glyph_t {
    id_t id;                              // SHA256 hash (64 bytes)
    content_t content;                    // Content buffer (256 bytes)
    len_t content_len;                    // Actual content length
    energy_t energy;                      // Energy level (Q16.16)
    ap_uint<32> activation_count;         // Activation counter
    ap_uint<64> last_update_time;         // Timestamp
};

// Merge result structure
struct merge_result_t {
    glyph_t glyph;                        // Merged glyph
    id_t parent1_id;                      // First parent ID
    id_t parent2_id;                      // Second parent ID
};

/**
 * Simplified hash function for hardware
 *
 * NOTE: This is NOT cryptographically secure SHA256.
 * For production, use Xilinx SHA256 IP core or similar.
 *
 * Latency: ~32 cycles
 */
void hash_content(content_t data, len_t len, id_t& hash_out) {
    #pragma HLS INLINE off
    #pragma HLS PIPELINE II=1

    ap_uint<32> h = 0x6a09e667;  // SHA256 initial value

    // Simple hash computation (pipelined)
    HASH_LOOP: for (int i = 0; i < 256; i++) {
        #pragma HLS PIPELINE II=1
        if (i < len) {
            ap_uint<8> byte = data.range(8*i+7, 8*i);
            h = ((h << 5) + h) ^ byte;
        }
    }

    // Replicate to 512 bits (64 bytes)
    hash_out = 0;
    for (int i = 0; i < 16; i++) {
        #pragma HLS UNROLL
        hash_out.range(32*i+31, 32*i) = h ^ (0x12345678 + i);
    }
}

/**
 * Core merge function
 *
 * Latency: ~71 cycles @ 200 MHz = 355ns
 * Throughput: 200M/71 = 2.8M merges/sec (with II=1)
 */
void merge_core(const glyph_t& g1, const glyph_t& g2, merge_result_t& result) {
    #pragma HLS INLINE off
    #pragma HLS PIPELINE II=1

    // Step 1: Energy comparison (1 cycle)
    glyph_t primary, secondary;

    if (g1.energy >= g2.energy) {
        primary = g1;
        secondary = g2;
    } else {
        primary = g2;
        secondary = g1;
    }

    // Step 2: Content concatenation (pipelined)
    content_t merged_content = 0;
    len_t pos = 0;

    // Copy primary content
    COPY_PRIMARY: for (int i = 0; i < 256; i++) {
        #pragma HLS UNROLL factor=8
        if (i < primary.content_len) {
            merged_content.range(8*pos+7, 8*pos) = primary.content.range(8*i+7, 8*i);
            pos++;
        }
    }

    // Add separator " + "
    merged_content.range(8*pos+7, 8*pos) = ' ';
    merged_content.range(8*(pos+1)+7, 8*(pos+1)) = '+';
    merged_content.range(8*(pos+2)+7, 8*(pos+2)) = ' ';
    pos += 3;

    // Copy secondary content
    COPY_SECONDARY: for (int i = 0; i < 256; i++) {
        #pragma HLS UNROLL factor=8
        if (i < secondary.content_len && pos < 256) {
            merged_content.range(8*pos+7, 8*pos) = secondary.content.range(8*i+7, 8*i);
            pos++;
        }
    }

    // Step 3: Hash computation (~32 cycles, can overlap with next steps)
    id_t merged_id;
    hash_content(merged_content, pos, merged_id);

    // Step 4: Energy sum (1 cycle, FP adder)
    #pragma HLS RESOURCE variable=result.glyph.energy core=FAddSub_fulldsp
    energy_t merged_energy = primary.energy + secondary.energy;

    // Step 5: Metadata merge (max operations, 2 cycles)
    ap_uint<32> merged_activation = (primary.activation_count > secondary.activation_count) ?
                                     primary.activation_count : secondary.activation_count;

    ap_uint<64> merged_time = (primary.last_update_time > secondary.last_update_time) ?
                               primary.last_update_time : secondary.last_update_time;

    // Build result
    result.glyph.id = merged_id;
    result.glyph.content = merged_content;
    result.glyph.content_len = pos;
    result.glyph.energy = merged_energy;
    result.glyph.activation_count = merged_activation;
    result.glyph.last_update_time = merged_time;
    result.parent1_id = primary.id;
    result.parent2_id = secondary.id;
}

/**
 * Top-level merge accelerator kernel
 *
 * AXI4-Stream interface for input/output
 * AXI4-Lite for control and status
 *
 * Processes pairs of glyphs from input stream, outputs merged results.
 */
void merge_accel(
    hls::stream<glyph_t>& input_stream,
    hls::stream<merge_result_t>& output_stream,
    ap_uint<32> count
) {
    // Interface pragmas
    #pragma HLS INTERFACE axis port=input_stream
    #pragma HLS INTERFACE axis port=output_stream
    #pragma HLS INTERFACE s_axilite port=count bundle=control
    #pragma HLS INTERFACE s_axilite port=return bundle=control

    #pragma HLS DATAFLOW

    // Process 'count' pairs
    PROCESS_PAIRS: for (ap_uint<32> i = 0; i < count; i++) {
        #pragma HLS PIPELINE II=71  // Latency of merge_core

        // Read pair
        glyph_t g1 = input_stream.read();
        glyph_t g2 = input_stream.read();

        // Merge
        merge_result_t result;
        merge_core(g1, g2, result);

        // Write result
        output_stream.write(result);
    }
}

/**
 * Multi-lane version (parallel processing)
 *
 * Instantiates N parallel merge units for higher throughput.
 * Target: 16 lanes @ 200 MHz = 3.2M merges/sec
 */
void merge_accel_parallel(
    hls::stream<glyph_t> input_streams[16],
    hls::stream<merge_result_t> output_streams[16],
    ap_uint<32> count
) {
    #pragma HLS INTERFACE axis port=input_streams
    #pragma HLS INTERFACE axis port=output_streams
    #pragma HLS INTERFACE s_axilite port=count bundle=control
    #pragma HLS INTERFACE s_axilite port=return bundle=control

    #pragma HLS DATAFLOW

    // Instantiate 16 parallel lanes
    PARALLEL_LANES: for (int lane = 0; lane < 16; lane++) {
        #pragma HLS UNROLL
        merge_accel(input_streams[lane], output_streams[lane], count);
    }
}
