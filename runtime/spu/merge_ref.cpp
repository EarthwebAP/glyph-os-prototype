/**
 * SPU Merge Primitive - C++ Reference Implementation
 *
 * Minimal, hardware-friendly implementation of glyph merge.
 * Designed for microbenchmarking and FPGA reference.
 */

#include "merge_ref.h"
#include <cstring>
#include <cstdio>
#include <algorithm>
#include <chrono>
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>

namespace spu {

// Constructor
Glyph::Glyph() {
    memset(id, 0, 65);
    memset(content, 0, 256);
    content_len = 0;
    energy = 0.0;
    activation_count = 0;
    last_update_time = 0;
    memset(parent1_id, 0, 65);
    memset(parent2_id, 0, 65);
}

/**
 * Simplified SHA256 for benchmarking
 * Note: In production, use OpenSSL or libsodium
 */
void sha256_hash(const char* data, size_t len, char* output) {
    // Simplified hash for benchmarking (NOT cryptographically secure)
    uint32_t h = 0x6a09e667;  // SHA256 initial value

    for (size_t i = 0; i < len; i++) {
        h = ((h << 5) + h) ^ static_cast<uint8_t>(data[i]);
    }

    // Convert to hex string (64 chars)
    snprintf(output, 65, "%08x%08x%08x%08x%08x%08x%08x%08x",
             h, h ^ 0x12345678, h ^ 0x9abcdef0, h ^ 0xfedcba98,
             h ^ 0x13579bdf, h ^ 0x2468ace0, h ^ 0x87654321, h ^ 0xabcdef01);
}

/**
 * Core merge implementation
 */
void merge(const Glyph& g1, const Glyph& g2, Glyph& result) {
    // Step 1: Determine precedence by energy
    const Glyph* primary;
    const Glyph* secondary;

    if (g1.energy >= g2.energy) {
        primary = &g1;
        secondary = &g2;
    } else {
        primary = &g2;
        secondary = &g1;
    }

    // Step 2: Concatenate content (primary + secondary)
    uint16_t pos = 0;

    // Copy primary content
    memcpy(result.content + pos, primary->content, primary->content_len);
    pos += primary->content_len;

    // Add separator " + "
    result.content[pos++] = ' ';
    result.content[pos++] = '+';
    result.content[pos++] = ' ';

    // Copy secondary content
    memcpy(result.content + pos, secondary->content, secondary->content_len);
    pos += secondary->content_len;

    result.content_len = pos;

    // Step 3: Compute ID via SHA256 hash
    sha256_hash(result.content, result.content_len, result.id);

    // Step 4: Sum energies
    result.energy = primary->energy + secondary->energy;

    // Step 5: Merge metadata (max operations)
    result.activation_count = std::max(primary->activation_count,
                                       secondary->activation_count);
    result.last_update_time = std::max(primary->last_update_time,
                                       secondary->last_update_time);

    // Step 6: Record provenance
    strncpy(result.parent1_id, primary->id, 64);
    strncpy(result.parent2_id, secondary->id, 64);
}

} // namespace spu

// Microbenchmark main
int main(int argc, char** argv) {
    using namespace std::chrono;
    using namespace spu;

    // Parse arguments
    int iterations = 100000;
    std::string output_file = "benchmarks/merge_ref_results.json";

    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        if (arg == "--iterations" && i + 1 < argc) {
            iterations = std::stoi(argv[++i]);
        } else if (arg == "--out" && i + 1 < argc) {
            output_file = argv[++i];
        }
    }

    std::cout << "=== SPU Merge Reference Microbenchmark ===\n";
    std::cout << "Iterations: " << iterations << "\n";
    std::cout << "Output: " << output_file << "\n\n";

    // Create test glyphs
    Glyph g1, g2, result;

    strncpy(g1.id, "id1_0000000000000000000000000000000000000000000000000000000000", 64);
    strncpy(g1.content, "content1", 8);
    g1.content_len = 8;
    g1.energy = 2.0;
    g1.activation_count = 0;
    g1.last_update_time = 0;

    strncpy(g2.id, "id2_0000000000000000000000000000000000000000000000000000000000", 64);
    strncpy(g2.content, "content2", 8);
    g2.content_len = 8;
    g2.energy = 3.0;
    g2.activation_count = 0;
    g2.last_update_time = 0;

    // Warmup
    std::cout << "Warming up...\n";
    for (int i = 0; i < 1000; i++) {
        merge(g1, g2, result);
    }

    // Benchmark
    std::cout << "Running benchmark...\n";
    std::vector<double> latencies;
    latencies.reserve(iterations);

    auto total_start = high_resolution_clock::now();

    for (int i = 0; i < iterations; i++) {
        auto start = high_resolution_clock::now();
        merge(g1, g2, result);
        auto end = high_resolution_clock::now();

        double latency_ns = duration_cast<nanoseconds>(end - start).count();
        latencies.push_back(latency_ns);
    }

    auto total_end = high_resolution_clock::now();
    auto total_duration_ns = duration_cast<nanoseconds>(total_end - total_start).count();

    // Compute statistics
    std::sort(latencies.begin(), latencies.end());

    double min_latency = latencies.front();
    double max_latency = latencies.back();
    double median_latency = latencies[latencies.size() / 2];
    double p95_latency = latencies[(latencies.size() * 95) / 100];
    double p99_latency = latencies[(latencies.size() * 99) / 100];

    double sum_latency = 0.0;
    for (double lat : latencies) {
        sum_latency += lat;
    }
    double mean_latency = sum_latency / latencies.size();

    double avg_latency_us = mean_latency / 1000.0;
    double ops_per_sec = 1e9 / mean_latency;

    // Print results
    std::cout << "\nResults:\n";
    std::cout << "--------\n";
    std::cout << "Min latency: " << min_latency << " ns\n";
    std::cout << "Max latency: " << max_latency << " ns\n";
    std::cout << "Median latency: " << median_latency << " ns\n";
    std::cout << "Mean latency: " << mean_latency << " ns (" << avg_latency_us << " µs)\n";
    std::cout << "P95 latency: " << p95_latency << " ns\n";
    std::cout << "P99 latency: " << p99_latency << " ns\n";
    std::cout << "Throughput: " << static_cast<int>(ops_per_sec) << " ops/sec\n";
    std::cout << "\n";

    // Compare to Python baseline (from bench_spu.py)
    double python_avg_us = 5.33;
    int python_ops_sec = 187652;
    double speedup = python_avg_us / avg_latency_us;

    std::cout << "vs Python baseline:\n";
    std::cout << "-------------------\n";
    std::cout << "Python: " << python_avg_us << " µs, " << python_ops_sec << " ops/sec\n";
    std::cout << "C++: " << avg_latency_us << " µs, " << static_cast<int>(ops_per_sec) << " ops/sec\n";
    std::cout << "Speedup: " << speedup << "x\n";
    std::cout << "\n";

    // Save JSON results (manual generation)
    std::ofstream out(output_file);
    out << "{\n";
    out << "  \"primitive\": \"merge\",\n";
    out << "  \"implementation\": \"cpp_reference\",\n";
    out << "  \"iterations\": " << iterations << ",\n";
    out << "  \"total_time_ns\": " << total_duration_ns << ",\n";
    out << "  \"latency_ns\": {\n";
    out << "    \"min\": " << min_latency << ",\n";
    out << "    \"max\": " << max_latency << ",\n";
    out << "    \"median\": " << median_latency << ",\n";
    out << "    \"mean\": " << mean_latency << ",\n";
    out << "    \"p95\": " << p95_latency << ",\n";
    out << "    \"p99\": " << p99_latency << "\n";
    out << "  },\n";
    out << "  \"latency_us\": {\n";
    out << "    \"mean\": " << avg_latency_us << "\n";
    out << "  },\n";
    out << "  \"throughput\": {\n";
    out << "    \"ops_per_sec\": " << static_cast<int>(ops_per_sec) << "\n";
    out << "  },\n";
    out << "  \"baseline_comparison\": {\n";
    out << "    \"python_avg_latency_us\": " << python_avg_us << ",\n";
    out << "    \"python_ops_per_sec\": " << python_ops_sec << ",\n";
    out << "    \"speedup\": " << speedup << "\n";
    out << "  }\n";
    out << "}\n";
    out.close();

    std::cout << "Results saved to: " << output_file << "\n";

    return 0;
}
