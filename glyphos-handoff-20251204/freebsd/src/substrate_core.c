/*
 * GlyphOS Phase 3: Substrate Core Implementation
 *
 * This module implements the deterministic field-state memory model that serves
 * as the physical substrate for GlyphOS operations. It provides:
 *
 * - Deterministic Field-State Memory Model (4096 cells)
 * - Substrate <-> CSE Handoff Protocol
 * - Parity checks and validation
 * - Musculature simulation stubs (ferrofluid dynamics)
 * - Quantum Pouch placeholder
 * - Comprehensive test mode
 *
 * Build: cc -o bin/substrate_core substrate_core.c -lm
 * Test:  ./bin/substrate_core --test
 *
 * Copyright (c) 2025 GlyphOS Project
 * FreeBSD Compatible - No external dependencies
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <math.h>
#include <time.h>
#include <stdbool.h>

/* ============================================================================
 * CONSTANTS AND CONFIGURATION
 * ============================================================================ */

#define SUBSTRATE_VERSION "1.0.0"
#define SUBSTRATE_CELL_COUNT 4096
#define M_PI 3.14159265358979323846

/* Phase wrapping: 0 to 2π */
#define PHASE_MIN 0.0
#define PHASE_MAX (2.0 * M_PI)

/* Coherence bounds: 0 to 1000 */
#define COHERENCE_MIN 0.0
#define COHERENCE_MAX 1000.0

/* Magnitude normalization */
#define MAGNITUDE_MIN 0.0
#define MAGNITUDE_MAX 1000.0

/* Decay rate limits */
#define DECAY_RATE_MIN 0.0
#define DECAY_RATE_MAX 1.0

/* Wave propagation constants */
#define WAVE_SPEED 1.0
#define WAVE_DAMPING 0.95
#define FERROFLUID_VISCOSITY 0.1

/* Quantum pouch superposition limit */
#define MAX_SUPERPOSITION_STATES 8

/* ============================================================================
 * DATA STRUCTURES
 * ============================================================================ */

/**
 * SubstrateCell - Individual cell in the field-state memory model
 *
 * Each cell represents a quantum-inspired field state with:
 * - magnitude: Field strength (0-1000)
 * - phase: Oscillation phase (0-2π radians)
 * - coherence: Quantum coherence measure (0-1000)
 * - decay_rate: Time-based decay coefficient (0-1)
 */
typedef struct {
    double magnitude;
    double phase;
    double coherence;
    double decay_rate;
    uint32_t last_update;  /* Timestamp of last modification */
    uint8_t flags;         /* Status flags */
} SubstrateCell;

/**
 * SubstrateState - Complete substrate system state
 */
typedef struct {
    SubstrateCell cells[SUBSTRATE_CELL_COUNT];
    uint64_t global_time;
    uint32_t checksum;
    uint32_t write_count;
    uint32_t read_count;
    bool initialized;
} SubstrateState;

/**
 * QuantumState - Superposition state for quantum pouch
 */
typedef struct {
    double amplitudes[MAX_SUPERPOSITION_STATES];
    double phases[MAX_SUPERPOSITION_STATES];
    uint8_t state_count;
    bool collapsed;
} QuantumState;

/**
 * WavePacket - Wave propagation data structure
 */
typedef struct {
    double amplitude;
    double frequency;
    double wavelength;
    double velocity;
    uint32_t origin_cell;
} WavePacket;

/* ============================================================================
 * GLOBAL STATE
 * ============================================================================ */

static SubstrateState g_substrate;

/* ============================================================================
 * UTILITY FUNCTIONS
 * ============================================================================ */

/**
 * normalize_phase - Wrap phase to [0, 2π]
 */
static double normalize_phase(double phase) {
    while (phase < PHASE_MIN) {
        phase += PHASE_MAX;
    }
    while (phase >= PHASE_MAX) {
        phase -= PHASE_MAX;
    }
    return phase;
}

/**
 * clamp_double - Clamp value to [min, max]
 */
static double clamp_double(double value, double min, double max) {
    if (value < min) return min;
    if (value > max) return max;
    return value;
}

/**
 * compute_checksum - Calculate checksum for substrate state
 */
static uint32_t compute_checksum(SubstrateState *state) {
    uint32_t sum = 0;
    for (int i = 0; i < SUBSTRATE_CELL_COUNT; i++) {
        /* Mix magnitude, phase, and coherence into checksum */
        uint32_t mag = (uint32_t)(state->cells[i].magnitude * 1000.0);
        uint32_t phs = (uint32_t)(state->cells[i].phase * 1000.0);
        uint32_t coh = (uint32_t)(state->cells[i].coherence * 1000.0);
        sum += (mag ^ phs ^ coh);
        sum = (sum << 1) | (sum >> 31);  /* Rotate left */
    }
    return sum;
}

/**
 * get_cell_neighbors - Get indices of neighboring cells (6-neighbor topology)
 */
static int get_cell_neighbors(uint32_t cell_idx, uint32_t *neighbors) {
    if (cell_idx >= SUBSTRATE_CELL_COUNT) {
        return 0;
    }

    int count = 0;
    uint32_t grid_width = 64;  /* 64x64 grid = 4096 cells */
    uint32_t x = cell_idx % grid_width;
    uint32_t y = cell_idx / grid_width;

    /* Left neighbor */
    if (x > 0) {
        neighbors[count++] = cell_idx - 1;
    }
    /* Right neighbor */
    if (x < grid_width - 1) {
        neighbors[count++] = cell_idx + 1;
    }
    /* Top neighbor */
    if (y > 0) {
        neighbors[count++] = cell_idx - grid_width;
    }
    /* Bottom neighbor */
    if (y < grid_width - 1) {
        neighbors[count++] = cell_idx + grid_width;
    }

    return count;
}

/* ============================================================================
 * SUBSTRATE INITIALIZATION
 * ============================================================================ */

/**
 * substrate_init - Initialize substrate to default state
 */
int substrate_init(void) {
    memset(&g_substrate, 0, sizeof(SubstrateState));

    /* Initialize all cells to neutral state */
    for (int i = 0; i < SUBSTRATE_CELL_COUNT; i++) {
        g_substrate.cells[i].magnitude = 100.0;
        g_substrate.cells[i].phase = 0.0;
        g_substrate.cells[i].coherence = 500.0;
        g_substrate.cells[i].decay_rate = 0.01;
        g_substrate.cells[i].last_update = 0;
        g_substrate.cells[i].flags = 0;
    }

    g_substrate.global_time = 0;
    g_substrate.write_count = 0;
    g_substrate.read_count = 0;
    g_substrate.initialized = true;
    g_substrate.checksum = compute_checksum(&g_substrate);

    return 0;
}

/**
 * substrate_reset - Reset substrate to initial state
 */
int substrate_reset(void) {
    return substrate_init();
}

/* ============================================================================
 * SUBSTRATE ↔ CSE HANDOFF PROTOCOL
 * ============================================================================ */

/**
 * substrate_read_cell - Read state from a substrate cell
 *
 * @param cell_idx Cell index (0-4095)
 * @param magnitude Output: field magnitude
 * @param phase Output: field phase
 * @param coherence Output: quantum coherence
 * @return 0 on success, -1 on error
 */
int substrate_read_cell(uint32_t cell_idx, double *magnitude, double *phase,
                       double *coherence) {
    if (!g_substrate.initialized) {
        fprintf(stderr, "Error: Substrate not initialized\n");
        return -1;
    }

    if (cell_idx >= SUBSTRATE_CELL_COUNT) {
        fprintf(stderr, "Error: Cell index %u out of bounds\n", cell_idx);
        return -1;
    }

    if (!magnitude || !phase || !coherence) {
        fprintf(stderr, "Error: NULL output pointer\n");
        return -1;
    }

    SubstrateCell *cell = &g_substrate.cells[cell_idx];

    *magnitude = cell->magnitude;
    *phase = cell->phase;
    *coherence = cell->coherence;

    g_substrate.read_count++;

    return 0;
}

/**
 * substrate_write_cell - Write state to a substrate cell
 *
 * @param cell_idx Cell index (0-4095)
 * @param magnitude Field magnitude (will be clamped)
 * @param phase Field phase (will be normalized to 0-2π)
 * @param coherence Quantum coherence (will be clamped)
 * @return 0 on success, -1 on error
 */
int substrate_write_cell(uint32_t cell_idx, double magnitude, double phase,
                        double coherence) {
    if (!g_substrate.initialized) {
        fprintf(stderr, "Error: Substrate not initialized\n");
        return -1;
    }

    if (cell_idx >= SUBSTRATE_CELL_COUNT) {
        fprintf(stderr, "Error: Cell index %u out of bounds\n", cell_idx);
        return -1;
    }

    SubstrateCell *cell = &g_substrate.cells[cell_idx];

    /* Apply parity checks and normalization */
    cell->magnitude = clamp_double(magnitude, MAGNITUDE_MIN, MAGNITUDE_MAX);
    cell->phase = normalize_phase(phase);
    cell->coherence = clamp_double(coherence, COHERENCE_MIN, COHERENCE_MAX);
    cell->last_update = (uint32_t)g_substrate.global_time;

    g_substrate.write_count++;
    g_substrate.checksum = compute_checksum(&g_substrate);

    return 0;
}

/**
 * substrate_sync - Synchronize substrate state (parity check)
 *
 * @return 0 on success, -1 on error
 */
int substrate_sync(void) {
    if (!g_substrate.initialized) {
        fprintf(stderr, "Error: Substrate not initialized\n");
        return -1;
    }

    /* Verify all cells are within bounds */
    for (int i = 0; i < SUBSTRATE_CELL_COUNT; i++) {
        SubstrateCell *cell = &g_substrate.cells[i];

        /* Parity check: phase wrapping */
        if (cell->phase < PHASE_MIN || cell->phase >= PHASE_MAX) {
            cell->phase = normalize_phase(cell->phase);
        }

        /* Parity check: coherence bounds */
        if (cell->coherence < COHERENCE_MIN || cell->coherence > COHERENCE_MAX) {
            cell->coherence = clamp_double(cell->coherence, COHERENCE_MIN,
                                          COHERENCE_MAX);
        }

        /* Parity check: magnitude normalization */
        if (cell->magnitude < MAGNITUDE_MIN || cell->magnitude > MAGNITUDE_MAX) {
            cell->magnitude = clamp_double(cell->magnitude, MAGNITUDE_MIN,
                                          MAGNITUDE_MAX);
        }

        /* Parity check: decay rate */
        if (cell->decay_rate < DECAY_RATE_MIN ||
            cell->decay_rate > DECAY_RATE_MAX) {
            cell->decay_rate = clamp_double(cell->decay_rate, DECAY_RATE_MIN,
                                           DECAY_RATE_MAX);
        }
    }

    /* Recompute checksum */
    uint32_t old_checksum = g_substrate.checksum;
    uint32_t new_checksum = compute_checksum(&g_substrate);
    g_substrate.checksum = new_checksum;

    if (old_checksum != new_checksum) {
        printf("Substrate sync: checksum updated 0x%08X -> 0x%08X\n",
               old_checksum, new_checksum);
    }

    return 0;
}

/**
 * substrate_tick - Advance substrate time by one tick
 */
void substrate_tick(void) {
    g_substrate.global_time++;

    /* Apply decay to all cells */
    for (int i = 0; i < SUBSTRATE_CELL_COUNT; i++) {
        SubstrateCell *cell = &g_substrate.cells[i];
        cell->magnitude *= (1.0 - cell->decay_rate);

        /* Prevent underflow */
        if (cell->magnitude < 0.01) {
            cell->magnitude = 0.01;
        }
    }
}

/* ============================================================================
 * MUSCULATURE SIMULATION (FERROFLUID DYNAMICS)
 * ============================================================================ */

/**
 * substrate_apply_force - Apply force vector to substrate cell
 *
 * Simulates ferrofluid response to external magnetic field
 *
 * @param cell_idx Target cell
 * @param force_x Force vector X component
 * @param force_y Force vector Y component
 * @param force_z Force vector Z component
 * @return 0 on success, -1 on error
 */
int substrate_apply_force(uint32_t cell_idx, double force_x, double force_y,
                         double force_z) {
    if (!g_substrate.initialized) {
        fprintf(stderr, "Error: Substrate not initialized\n");
        return -1;
    }

    if (cell_idx >= SUBSTRATE_CELL_COUNT) {
        fprintf(stderr, "Error: Cell index %u out of bounds\n", cell_idx);
        return -1;
    }

    SubstrateCell *cell = &g_substrate.cells[cell_idx];

    /* Compute force magnitude */
    double force_mag = sqrt(force_x * force_x + force_y * force_y +
                           force_z * force_z);

    /* Update cell magnitude based on force */
    cell->magnitude += force_mag * (1.0 - FERROFLUID_VISCOSITY);
    cell->magnitude = clamp_double(cell->magnitude, MAGNITUDE_MIN,
                                   MAGNITUDE_MAX);

    /* Update phase based on force direction */
    double force_angle = atan2(force_y, force_x);
    cell->phase = normalize_phase(cell->phase + force_angle * 0.1);

    /* Increase coherence with applied force */
    cell->coherence += force_mag * 0.5;
    cell->coherence = clamp_double(cell->coherence, COHERENCE_MIN,
                                   COHERENCE_MAX);

    cell->last_update = (uint32_t)g_substrate.global_time;

    return 0;
}

/**
 * substrate_propagate_wave - Propagate wave through substrate
 *
 * Simulates ferrofluid wave dynamics with damping
 *
 * @param origin_cell Cell where wave originates
 * @param wave_amplitude Initial wave amplitude
 * @param wave_frequency Wave frequency
 * @return 0 on success, -1 on error
 */
int substrate_propagate_wave(uint32_t origin_cell, double wave_amplitude,
                            double wave_frequency) {
    if (!g_substrate.initialized) {
        fprintf(stderr, "Error: Substrate not initialized\n");
        return -1;
    }

    if (origin_cell >= SUBSTRATE_CELL_COUNT) {
        fprintf(stderr, "Error: Cell index %u out of bounds\n", origin_cell);
        return -1;
    }

    /* Create wave packet */
    WavePacket wave;
    wave.amplitude = wave_amplitude;
    wave.frequency = wave_frequency;
    wave.wavelength = WAVE_SPEED / wave_frequency;
    wave.velocity = WAVE_SPEED;
    wave.origin_cell = origin_cell;

    /* Propagate wave to neighbors using breadth-first approach */
    bool visited[SUBSTRATE_CELL_COUNT] = {false};
    uint32_t queue[SUBSTRATE_CELL_COUNT];
    double distances[SUBSTRATE_CELL_COUNT];
    int queue_head = 0, queue_tail = 0;

    queue[queue_tail++] = origin_cell;
    visited[origin_cell] = true;
    distances[origin_cell] = 0.0;

    while (queue_head < queue_tail) {
        uint32_t current = queue[queue_head++];
        double distance = distances[current];

        /* Calculate wave attenuation */
        double attenuation = pow(WAVE_DAMPING, distance);
        double phase_shift = 2.0 * M_PI * distance / wave.wavelength;

        /* Apply wave to current cell */
        SubstrateCell *cell = &g_substrate.cells[current];
        double wave_contribution = wave.amplitude * attenuation *
                                  cos(wave.frequency * g_substrate.global_time +
                                      phase_shift);

        cell->magnitude += fabs(wave_contribution);
        cell->magnitude = clamp_double(cell->magnitude, MAGNITUDE_MIN,
                                      MAGNITUDE_MAX);
        cell->phase = normalize_phase(cell->phase + phase_shift);

        /* Add neighbors to queue */
        uint32_t neighbors[4];
        int neighbor_count = get_cell_neighbors(current, neighbors);

        for (int i = 0; i < neighbor_count; i++) {
            uint32_t neighbor = neighbors[i];
            if (!visited[neighbor] && queue_tail < SUBSTRATE_CELL_COUNT) {
                visited[neighbor] = true;
                queue[queue_tail++] = neighbor;
                distances[neighbor] = distance + 1.0;
            }
        }

        /* Limit wave propagation distance */
        if (distance > 10.0) {
            break;
        }
    }

    return 0;
}

/* ============================================================================
 * QUANTUM POUCH (PLACEHOLDER)
 * ============================================================================ */

/**
 * substrate_quantum_store - Store quantum superposition state
 *
 * @param cell_idx Target cell
 * @param state Quantum state to store
 * @return 0 on success, -1 on error
 */
int substrate_quantum_store(uint32_t cell_idx, QuantumState *state) {
    if (!g_substrate.initialized) {
        fprintf(stderr, "Error: Substrate not initialized\n");
        return -1;
    }

    if (cell_idx >= SUBSTRATE_CELL_COUNT) {
        fprintf(stderr, "Error: Cell index %u out of bounds\n", cell_idx);
        return -1;
    }

    if (!state) {
        fprintf(stderr, "Error: NULL quantum state\n");
        return -1;
    }

    if (state->state_count > MAX_SUPERPOSITION_STATES) {
        fprintf(stderr, "Error: Too many superposition states\n");
        return -1;
    }

    SubstrateCell *cell = &g_substrate.cells[cell_idx];

    /* Compute average magnitude and phase from superposition */
    double avg_magnitude = 0.0;
    double avg_phase = 0.0;
    double total_amplitude = 0.0;

    for (int i = 0; i < state->state_count; i++) {
        double amp = state->amplitudes[i];
        total_amplitude += amp;
        avg_magnitude += amp * 100.0;  /* Scale to substrate magnitude */
        avg_phase += state->phases[i];
    }

    if (total_amplitude > 0.0) {
        avg_magnitude /= total_amplitude;
        avg_phase /= (double)state->state_count;
    }

    /* Store superposition as field state */
    cell->magnitude = clamp_double(avg_magnitude, MAGNITUDE_MIN, MAGNITUDE_MAX);
    cell->phase = normalize_phase(avg_phase);
    cell->coherence = state->collapsed ? 0.0 : COHERENCE_MAX;
    cell->flags |= 0x01;  /* Mark as quantum cell */

    return 0;
}

/**
 * substrate_quantum_retrieve - Retrieve quantum superposition state
 *
 * @param cell_idx Source cell
 * @param state Output quantum state
 * @return 0 on success, -1 on error
 */
int substrate_quantum_retrieve(uint32_t cell_idx, QuantumState *state) {
    if (!g_substrate.initialized) {
        fprintf(stderr, "Error: Substrate not initialized\n");
        return -1;
    }

    if (cell_idx >= SUBSTRATE_CELL_COUNT) {
        fprintf(stderr, "Error: Cell index %u out of bounds\n", cell_idx);
        return -1;
    }

    if (!state) {
        fprintf(stderr, "Error: NULL quantum state\n");
        return -1;
    }

    SubstrateCell *cell = &g_substrate.cells[cell_idx];

    /* Check if cell contains quantum data */
    if (!(cell->flags & 0x01)) {
        fprintf(stderr, "Warning: Cell %u is not marked as quantum\n", cell_idx);
    }

    /* Reconstruct superposition from field state */
    /* This is a placeholder - real quantum reconstruction would be complex */
    state->state_count = 1;
    state->amplitudes[0] = cell->magnitude / 100.0;
    state->phases[0] = cell->phase;
    state->collapsed = (cell->coherence < 1.0);

    return 0;
}

/* ============================================================================
 * STATUS AND DIAGNOSTICS
 * ============================================================================ */

/**
 * substrate_print_status - Print substrate status information
 */
void substrate_print_status(void) {
    if (!g_substrate.initialized) {
        printf("Substrate Status: NOT INITIALIZED\n");
        return;
    }

    printf("\n");
    printf("=== Substrate Core Status ===\n");
    printf("Version:        %s\n", SUBSTRATE_VERSION);
    printf("Initialized:    %s\n", g_substrate.initialized ? "YES" : "NO");
    printf("Cell Count:     %d\n", SUBSTRATE_CELL_COUNT);
    printf("Global Time:    %lu\n", (unsigned long)g_substrate.global_time);
    printf("Checksum:       0x%08X\n", g_substrate.checksum);
    printf("Read Ops:       %u\n", g_substrate.read_count);
    printf("Write Ops:      %u\n", g_substrate.write_count);

    /* Calculate statistics */
    double total_magnitude = 0.0;
    double total_coherence = 0.0;
    double max_magnitude = 0.0;

    for (int i = 0; i < SUBSTRATE_CELL_COUNT; i++) {
        total_magnitude += g_substrate.cells[i].magnitude;
        total_coherence += g_substrate.cells[i].coherence;
        if (g_substrate.cells[i].magnitude > max_magnitude) {
            max_magnitude = g_substrate.cells[i].magnitude;
        }
    }

    double avg_magnitude = total_magnitude / SUBSTRATE_CELL_COUNT;
    double avg_coherence = total_coherence / SUBSTRATE_CELL_COUNT;

    printf("Avg Magnitude:  %.2f\n", avg_magnitude);
    printf("Max Magnitude:  %.2f\n", max_magnitude);
    printf("Avg Coherence:  %.2f\n", avg_coherence);
    printf("=============================\n");
    printf("\n");
}

/* ============================================================================
 * TEST MODE
 * ============================================================================ */

/**
 * test_initialization - Test substrate initialization
 */
static bool test_initialization(void) {
    printf("Test 1: Substrate Initialization... ");

    int result = substrate_init();
    if (result != 0) {
        printf("FAIL (init returned %d)\n", result);
        return false;
    }

    if (!g_substrate.initialized) {
        printf("FAIL (not marked initialized)\n");
        return false;
    }

    if (g_substrate.global_time != 0) {
        printf("FAIL (global time not zero)\n");
        return false;
    }

    /* Check first cell */
    if (g_substrate.cells[0].magnitude != 100.0) {
        printf("FAIL (incorrect initial magnitude)\n");
        return false;
    }

    printf("PASS\n");
    return true;
}

/**
 * test_read_write - Test cell read/write operations
 */
static bool test_read_write(void) {
    printf("Test 2: Cell Read/Write... ");

    uint32_t test_cell = 100;
    double write_mag = 250.0;
    double write_phase = M_PI;
    double write_coh = 750.0;

    /* Write to cell */
    int result = substrate_write_cell(test_cell, write_mag, write_phase,
                                     write_coh);
    if (result != 0) {
        printf("FAIL (write returned %d)\n", result);
        return false;
    }

    /* Read from cell */
    double read_mag, read_phase, read_coh;
    result = substrate_read_cell(test_cell, &read_mag, &read_phase, &read_coh);
    if (result != 0) {
        printf("FAIL (read returned %d)\n", result);
        return false;
    }

    /* Verify values */
    if (fabs(read_mag - write_mag) > 0.001) {
        printf("FAIL (magnitude mismatch: %.2f != %.2f)\n", read_mag, write_mag);
        return false;
    }

    if (fabs(read_phase - write_phase) > 0.001) {
        printf("FAIL (phase mismatch: %.2f != %.2f)\n", read_phase, write_phase);
        return false;
    }

    if (fabs(read_coh - write_coh) > 0.001) {
        printf("FAIL (coherence mismatch: %.2f != %.2f)\n", read_coh, write_coh);
        return false;
    }

    printf("PASS\n");
    return true;
}

/**
 * test_parity_checks - Test parity checks and normalization
 */
static bool test_parity_checks(void) {
    printf("Test 3: Parity Checks... ");

    uint32_t test_cell = 200;

    /* Test phase wrapping (write 3π, should wrap to π) */
    substrate_write_cell(test_cell, 100.0, 3.0 * M_PI, 500.0);
    double mag, phase, coh;
    substrate_read_cell(test_cell, &mag, &phase, &coh);

    if (phase < PHASE_MIN || phase >= PHASE_MAX) {
        printf("FAIL (phase not wrapped: %.2f)\n", phase);
        return false;
    }

    /* Test magnitude clamping (write 2000, should clamp to 1000) */
    substrate_write_cell(test_cell, 2000.0, 0.0, 500.0);
    substrate_read_cell(test_cell, &mag, &phase, &coh);

    if (mag > MAGNITUDE_MAX) {
        printf("FAIL (magnitude not clamped: %.2f)\n", mag);
        return false;
    }

    /* Test coherence clamping (write 2000, should clamp to 1000) */
    substrate_write_cell(test_cell, 100.0, 0.0, 2000.0);
    substrate_read_cell(test_cell, &mag, &phase, &coh);

    if (coh > COHERENCE_MAX) {
        printf("FAIL (coherence not clamped: %.2f)\n", coh);
        return false;
    }

    /* Test sync function */
    int result = substrate_sync();
    if (result != 0) {
        printf("FAIL (sync returned %d)\n", result);
        return false;
    }

    printf("PASS\n");
    return true;
}

/**
 * test_wave_propagation - Test wave propagation simulation
 */
static bool test_wave_propagation(void) {
    printf("Test 4: Wave Propagation... ");

    /* Reset substrate */
    substrate_reset();

    uint32_t origin = 2048;  /* Center of grid */
    double initial_mag = g_substrate.cells[origin].magnitude;

    /* Propagate wave */
    int result = substrate_propagate_wave(origin, 50.0, 1.0);
    if (result != 0) {
        printf("FAIL (propagate returned %d)\n", result);
        return false;
    }

    /* Check that origin cell was affected */
    double final_mag = g_substrate.cells[origin].magnitude;
    if (final_mag <= initial_mag) {
        printf("FAIL (origin cell not affected: %.2f <= %.2f)\n",
               final_mag, initial_mag);
        return false;
    }

    /* Check that neighbors were affected */
    uint32_t neighbors[4];
    int neighbor_count = get_cell_neighbors(origin, neighbors);

    if (neighbor_count == 0) {
        printf("FAIL (no neighbors found)\n");
        return false;
    }

    bool neighbor_affected = false;
    for (int i = 0; i < neighbor_count; i++) {
        if (g_substrate.cells[neighbors[i]].magnitude > 100.0) {
            neighbor_affected = true;
            break;
        }
    }

    if (!neighbor_affected) {
        printf("FAIL (neighbors not affected)\n");
        return false;
    }

    printf("PASS\n");
    return true;
}

/**
 * test_force_application - Test force application
 */
static bool test_force_application(void) {
    printf("Test 5: Force Application... ");

    substrate_reset();

    uint32_t test_cell = 500;
    double initial_mag = g_substrate.cells[test_cell].magnitude;

    /* Apply force */
    int result = substrate_apply_force(test_cell, 10.0, 10.0, 10.0);
    if (result != 0) {
        printf("FAIL (apply_force returned %d)\n", result);
        return false;
    }

    /* Check that cell was affected */
    double final_mag = g_substrate.cells[test_cell].magnitude;
    if (final_mag <= initial_mag) {
        printf("FAIL (cell not affected: %.2f <= %.2f)\n",
               final_mag, initial_mag);
        return false;
    }

    printf("PASS\n");
    return true;
}

/**
 * test_quantum_pouch - Test quantum pouch operations
 */
static bool test_quantum_pouch(void) {
    printf("Test 6: Quantum Pouch... ");

    uint32_t test_cell = 1000;

    /* Create quantum state */
    QuantumState state;
    state.state_count = 3;
    state.amplitudes[0] = 0.5;
    state.amplitudes[1] = 0.3;
    state.amplitudes[2] = 0.2;
    state.phases[0] = 0.0;
    state.phases[1] = M_PI / 2.0;
    state.phases[2] = M_PI;
    state.collapsed = false;

    /* Store quantum state */
    int result = substrate_quantum_store(test_cell, &state);
    if (result != 0) {
        printf("FAIL (quantum_store returned %d)\n", result);
        return false;
    }

    /* Retrieve quantum state */
    QuantumState retrieved;
    result = substrate_quantum_retrieve(test_cell, &retrieved);
    if (result != 0) {
        printf("FAIL (quantum_retrieve returned %d)\n", result);
        return false;
    }

    /* Verify retrieval */
    if (retrieved.state_count < 1) {
        printf("FAIL (no states retrieved)\n");
        return false;
    }

    printf("PASS\n");
    return true;
}

/**
 * run_all_tests - Run complete test suite
 */
static int run_all_tests(void) {
    printf("\n");
    printf("=================================\n");
    printf("GlyphOS Substrate Core Test Suite\n");
    printf("=================================\n");
    printf("\n");

    int passed = 0;
    int total = 6;

    if (test_initialization()) passed++;
    if (test_read_write()) passed++;
    if (test_parity_checks()) passed++;
    if (test_wave_propagation()) passed++;
    if (test_force_application()) passed++;
    if (test_quantum_pouch()) passed++;

    printf("\n");
    printf("=================================\n");
    printf("Results: %d/%d tests passed\n", passed, total);
    printf("=================================\n");
    printf("\n");

    return (passed == total) ? 0 : 1;
}

/* ============================================================================
 * MAIN PROGRAM
 * ============================================================================ */

static void print_usage(const char *prog_name) {
    printf("GlyphOS Substrate Core v%s\n", SUBSTRATE_VERSION);
    printf("\n");
    printf("Usage: %s [OPTIONS]\n", prog_name);
    printf("\n");
    printf("Options:\n");
    printf("  --test        Run comprehensive test suite\n");
    printf("  --status      Print substrate status\n");
    printf("  --help        Display this help message\n");
    printf("\n");
    printf("Examples:\n");
    printf("  %s --test          # Run all tests\n", prog_name);
    printf("  %s --status        # Show substrate status\n", prog_name);
    printf("\n");
    printf("Build: cc -o bin/substrate_core substrate_core.c -lm\n");
    printf("\n");
}

int main(int argc, char *argv[]) {
    /* Parse command line arguments */
    if (argc < 2) {
        print_usage(argv[0]);
        return 1;
    }

    if (strcmp(argv[1], "--test") == 0) {
        /* Run test suite */
        return run_all_tests();
    }
    else if (strcmp(argv[1], "--status") == 0) {
        /* Initialize and print status */
        substrate_init();
        substrate_print_status();
        return 0;
    }
    else if (strcmp(argv[1], "--help") == 0) {
        print_usage(argv[0]);
        return 0;
    }
    else {
        fprintf(stderr, "Error: Unknown option '%s'\n", argv[1]);
        fprintf(stderr, "Use --help for usage information\n");
        return 1;
    }

    return 0;
}
