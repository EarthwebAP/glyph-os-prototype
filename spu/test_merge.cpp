/**
 * Test/benchmark program for SPU merge reference implementation
 */

#include <iostream>
#include <chrono>
#include <cstring>
#include "merge_reference.cpp"

using namespace spu;
using namespace std;
using namespace std::chrono;

// Helper to create test glyph
Glyph make_glyph(const char* id, const char* content, double energy) {
    Glyph g;
    strncpy(g.id, id, 64);
    strncpy(g.content, content, 256);
    g.content_len = strlen(content);
    g.energy = energy;
    g.activation_count = 0;
    g.last_update_time = 0;
    return g;
}

// Print glyph for debugging
void print_glyph(const Glyph& g, const char* label) {
    cout << label << ":\n";
    cout << "  ID: " << g.id << "\n";
    cout << "  Content: " << string(g.content, g.content_len) << "\n";
    cout << "  Energy: " << g.energy << "\n";
    cout << "  Activation count: " << g.activation_count << "\n";
    cout << "  Last update: " << g.last_update_time << "\n";
}

int main() {
    cout << "=== SPU Merge Reference Implementation Test ===\n\n";

    // Test 1: Basic merge
    cout << "Test 1: Basic merge (higher energy wins)\n";
    cout << "==========================================\n";

    Glyph g1 = make_glyph("id1", "content1", 2.0);
    Glyph g2 = make_glyph("id2", "content2", 3.0);
    Glyph result;

    print_glyph(g1, "Glyph 1");
    print_glyph(g2, "Glyph 2");

    merge(g1, g2, result);

    print_glyph(result, "Merged Result");
    cout << "  Parent 1 ID: " << result.parent1_id << "\n";
    cout << "  Parent 2 ID: " << result.parent2_id << "\n";
    cout << "\n";

    // Test 2: Energy conservation
    cout << "Test 2: Energy conservation\n";
    cout << "===========================\n";
    double expected_energy = g1.energy + g2.energy;
    double actual_energy = result.energy;
    cout << "  Expected energy: " << expected_energy << "\n";
    cout << "  Actual energy: " << actual_energy << "\n";
    cout << "  Conservation: " << (expected_energy == actual_energy ? "PASS" : "FAIL") << "\n";
    cout << "\n";

    // Test 3: Precedence (energy order)
    cout << "Test 3: Content precedence\n";
    cout << "==========================\n";
    string result_content(result.content, result.content_len);
    bool primary_first = result_content.find("content2") < result_content.find("content1");
    cout << "  Result content: " << result_content << "\n";
    cout << "  Higher energy first: " << (primary_first ? "PASS" : "FAIL") << "\n";
    cout << "\n";

    // Test 4: Performance benchmark
    cout << "Test 4: Performance benchmark\n";
    cout << "=============================\n";

    const int iterations = 100000;
    Glyph bench_g1 = make_glyph("bench1", "benchmark content 1", 5.5);
    Glyph bench_g2 = make_glyph("bench2", "benchmark content 2", 3.2);
    Glyph bench_result;

    auto start = high_resolution_clock::now();

    for (int i = 0; i < iterations; i++) {
        merge(bench_g1, bench_g2, bench_result);
    }

    auto end = high_resolution_clock::now();
    auto duration = duration_cast<nanoseconds>(end - start).count();

    double avg_latency_ns = static_cast<double>(duration) / iterations;
    double avg_latency_us = avg_latency_ns / 1000.0;
    double ops_per_sec = 1e9 / avg_latency_ns;

    cout << "  Iterations: " << iterations << "\n";
    cout << "  Total time: " << duration / 1e6 << " ms\n";
    cout << "  Average latency: " << avg_latency_ns << " ns (" << avg_latency_us << " µs)\n";
    cout << "  Throughput: " << static_cast<int>(ops_per_sec) << " ops/sec\n";
    cout << "\n";

    // Compare to Python baseline
    cout << "Comparison to Python baseline:\n";
    cout << "==============================\n";
    double python_latency_us = 5.33;
    double python_ops_per_sec = 187652;
    double speedup = python_latency_us / avg_latency_us;

    cout << "  Python latency: " << python_latency_us << " µs\n";
    cout << "  C++ latency: " << avg_latency_us << " µs\n";
    cout << "  Speedup: " << speedup << "x\n";
    cout << "  Python throughput: " << python_ops_per_sec << " ops/sec\n";
    cout << "  C++ throughput: " << static_cast<int>(ops_per_sec) << " ops/sec\n";
    cout << "\n";

    // Test 5: Batch processing
    cout << "Test 5: Batch processing (simulated)\n";
    cout << "=====================================\n";

    const int batch_size = 1000;
    Glyph pairs[batch_size * 2];
    Glyph results[batch_size];

    // Initialize pairs
    for (int i = 0; i < batch_size; i++) {
        pairs[i*2] = make_glyph("batch_a", "content_a", 2.0 + i * 0.1);
        pairs[i*2+1] = make_glyph("batch_b", "content_b", 3.0 + i * 0.1);
    }

    start = high_resolution_clock::now();
    merge_batch(pairs, results, batch_size);
    end = high_resolution_clock::now();

    auto batch_duration = duration_cast<microseconds>(end - start).count();
    double batch_throughput = (batch_size * 1e6) / batch_duration;

    cout << "  Batch size: " << batch_size << "\n";
    cout << "  Total time: " << batch_duration << " µs\n";
    cout << "  Throughput: " << static_cast<int>(batch_throughput) << " ops/sec\n";
    cout << "\n";

    cout << "=== All Tests Complete ===\n";

    return 0;
}
