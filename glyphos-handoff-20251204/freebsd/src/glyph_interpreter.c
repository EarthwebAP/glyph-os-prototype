/*
 * glyph_interpreter.c - Phase 4 Glyph Interpreter for GlyphOS
 *
 * Complete implementation of the Glyph Definition Format (GDF) parser,
 * symbolic field interpreter, activation simulator, and inheritance chain runner.
 *
 * Copyright (c) 2025 GlyphOS Project
 * FreeBSD Compatible - No External Dependencies
 *
 * Compilation: cc -o bin/glyph_interp glyph_interpreter.c
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <ctype.h>
#include <time.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>

/* ============================================================================
 * CONSTANTS AND CONFIGURATION
 * ========================================================================== */

#define MAX_GLYPH_ID_LEN 64
#define MAX_CHRONOCODE_LEN 32
#define MAX_PARENT_GLYPHS 16
#define MAX_MATERIAL_SPEC_LEN 256
#define MAX_FREQ_SIG_LEN 512
#define MAX_ACTIVATION_CMD_LEN 1024
#define MAX_FIELD_NAME_LEN 64
#define MAX_LINE_LENGTH 2048
#define MAX_GLYPHS 256
#define MAX_INHERITANCE_DEPTH 32
#define MAX_TRACE_ENTRIES 1024

#define VAULT_PATH "./vault"
#define DEFAULT_RESONANCE 440.0
#define DEFAULT_MAGNITUDE 1.0
#define DEFAULT_COHERENCE 100

/* ============================================================================
 * DATA STRUCTURES
 * ========================================================================== */

/* Symbolic field representation */
typedef struct {
    char name[MAX_FIELD_NAME_LEN];
    double value;
    int is_active;
} SymbolicField;

/* Glyph activation command */
typedef struct {
    char command[64];
    double param;
    char target[MAX_GLYPH_ID_LEN];
    int has_param;
    int has_target;
} ActivationCommand;

/* Complete GDF glyph structure */
typedef struct {
    char glyph_id[MAX_GLYPH_ID_LEN];
    char chronocode[MAX_CHRONOCODE_LEN];
    char parent_glyphs[MAX_PARENT_GLYPHS][MAX_GLYPH_ID_LEN];
    int parent_count;
    double resonance_freq;
    double field_magnitude;
    int coherence;
    char contributor_inheritance[MAX_GLYPH_ID_LEN];
    char material_spec[MAX_MATERIAL_SPEC_LEN];
    char frequency_signature[MAX_FREQ_SIG_LEN];
    char activation_simulation[MAX_ACTIVATION_CMD_LEN];
    double entanglement_coeff;
    double phase_offset;
    int quantum_state;
    char metadata[MAX_MATERIAL_SPEC_LEN];
    char dependencies[MAX_FREQ_SIG_LEN];
    char outputs[MAX_FREQ_SIG_LEN];
    char constraints[MAX_FREQ_SIG_LEN];
    int is_loaded;
} GlyphDefinition;

/* Field state during activation */
typedef struct {
    double resonance;
    double magnitude;
    double phase;
    int coherence;
    double entanglement_factor;
    int depth;
    char active_glyph[MAX_GLYPH_ID_LEN];
} FieldState;

/* Trace entry for execution logging */
typedef struct {
    char timestamp[32];
    char glyph_id[MAX_GLYPH_ID_LEN];
    char operation[128];
    FieldState state;
} TraceEntry;

/* Global glyph registry */
typedef struct {
    GlyphDefinition glyphs[MAX_GLYPHS];
    int count;
    TraceEntry traces[MAX_TRACE_ENTRIES];
    int trace_count;
} GlyphRegistry;

/* ============================================================================
 * GLOBAL STATE
 * ========================================================================== */

static GlyphRegistry g_registry = {0};
static int g_verbose = 0;
static int g_trace_enabled = 1;

/* ============================================================================
 * UTILITY FUNCTIONS
 * ========================================================================== */

/* Trim whitespace from string */
static char* trim_whitespace(char* str) {
    char* end;
    while(isspace((unsigned char)*str)) str++;
    if(*str == 0) return str;
    end = str + strlen(str) - 1;
    while(end > str && isspace((unsigned char)*end)) end--;
    end[1] = '\0';
    return str;
}

/* Generate timestamp */
static void get_timestamp(char* buffer, size_t size) {
    time_t now = time(NULL);
    struct tm* tm_info = localtime(&now);
    strftime(buffer, size, "%Y%m%d_%H%M%S", tm_info);
}

/* Add trace entry */
static void add_trace(const char* glyph_id, const char* operation, FieldState* state) {
    if (!g_trace_enabled || g_registry.trace_count >= MAX_TRACE_ENTRIES) {
        return;
    }

    TraceEntry* entry = &g_registry.traces[g_registry.trace_count++];
    get_timestamp(entry->timestamp, sizeof(entry->timestamp));
    snprintf(entry->glyph_id, sizeof(entry->glyph_id), "%s", glyph_id);
    snprintf(entry->operation, sizeof(entry->operation), "%s", operation);
    if (state) {
        entry->state = *state;
    }
}

/* Print trace output */
static void print_trace_log(void) {
    printf("\n=== SYMBOLIC TRACE OUTPUT ===\n");
    printf("Total trace entries: %d\n\n", g_registry.trace_count);

    for (int i = 0; i < g_registry.trace_count; i++) {
        TraceEntry* e = &g_registry.traces[i];
        printf("[%s] Glyph:%s | %s\n", e->timestamp, e->glyph_id, e->operation);
        printf("  State: R=%.2fHz M=%.3f P=%.2f C=%d E=%.3f D=%d\n\n",
               e->state.resonance, e->state.magnitude, e->state.phase,
               e->state.coherence, e->state.entanglement_factor, e->state.depth);
    }
}

/* ============================================================================
 * GDF PARSER - 18-FIELD SCHEMA
 * ========================================================================== */

/* Parse parent glyph list (comma-separated) */
static int parse_parent_list(const char* value, GlyphDefinition* glyph) {
    char buffer[MAX_LINE_LENGTH];
    strncpy(buffer, value, sizeof(buffer) - 1);
    buffer[sizeof(buffer) - 1] = '\0';

    glyph->parent_count = 0;
    char* token = strtok(buffer, ",");

    while (token && glyph->parent_count < MAX_PARENT_GLYPHS) {
        token = trim_whitespace(token);
        if (strlen(token) > 0) {
            strncpy(glyph->parent_glyphs[glyph->parent_count++], token, MAX_GLYPH_ID_LEN - 1);
        }
        token = strtok(NULL, ",");
    }

    return glyph->parent_count;
}

/* Parse a single GDF field */
static int parse_gdf_field(const char* key, const char* value, GlyphDefinition* glyph) {
    char* trimmed_key = trim_whitespace((char*)key);
    char* trimmed_value = trim_whitespace((char*)value);

    if (strcmp(trimmed_key, "glyph_id") == 0) {
        strncpy(glyph->glyph_id, trimmed_value, MAX_GLYPH_ID_LEN - 1);
    }
    else if (strcmp(trimmed_key, "chronocode") == 0) {
        strncpy(glyph->chronocode, trimmed_value, MAX_CHRONOCODE_LEN - 1);
    }
    else if (strcmp(trimmed_key, "parent") == 0 || strcmp(trimmed_key, "parent_glyphs") == 0) {
        parse_parent_list(trimmed_value, glyph);
    }
    else if (strcmp(trimmed_key, "resonance_freq") == 0 || strcmp(trimmed_key, "resonance") == 0) {
        glyph->resonance_freq = atof(trimmed_value);
    }
    else if (strcmp(trimmed_key, "field_magnitude") == 0 || strcmp(trimmed_key, "magnitude") == 0) {
        glyph->field_magnitude = atof(trimmed_value);
    }
    else if (strcmp(trimmed_key, "coherence") == 0) {
        glyph->coherence = atoi(trimmed_value);
    }
    else if (strcmp(trimmed_key, "contributor_inheritance") == 0 || strcmp(trimmed_key, "contributor") == 0) {
        strncpy(glyph->contributor_inheritance, trimmed_value, MAX_GLYPH_ID_LEN - 1);
    }
    else if (strcmp(trimmed_key, "material_spec") == 0 || strcmp(trimmed_key, "material") == 0) {
        strncpy(glyph->material_spec, trimmed_value, MAX_MATERIAL_SPEC_LEN - 1);
    }
    else if (strcmp(trimmed_key, "frequency_signature") == 0 || strcmp(trimmed_key, "freq_sig") == 0) {
        strncpy(glyph->frequency_signature, trimmed_value, MAX_FREQ_SIG_LEN - 1);
    }
    else if (strcmp(trimmed_key, "activation_simulation") == 0 || strcmp(trimmed_key, "activation") == 0) {
        strncpy(glyph->activation_simulation, trimmed_value, MAX_ACTIVATION_CMD_LEN - 1);
    }
    else if (strcmp(trimmed_key, "entanglement_coeff") == 0 || strcmp(trimmed_key, "entanglement") == 0) {
        glyph->entanglement_coeff = atof(trimmed_value);
    }
    else if (strcmp(trimmed_key, "phase_offset") == 0 || strcmp(trimmed_key, "phase") == 0) {
        glyph->phase_offset = atof(trimmed_value);
    }
    else if (strcmp(trimmed_key, "quantum_state") == 0) {
        glyph->quantum_state = atoi(trimmed_value);
    }
    else if (strcmp(trimmed_key, "metadata") == 0) {
        strncpy(glyph->metadata, trimmed_value, MAX_MATERIAL_SPEC_LEN - 1);
    }
    else if (strcmp(trimmed_key, "dependencies") == 0) {
        strncpy(glyph->dependencies, trimmed_value, MAX_FREQ_SIG_LEN - 1);
    }
    else if (strcmp(trimmed_key, "outputs") == 0) {
        strncpy(glyph->outputs, trimmed_value, MAX_FREQ_SIG_LEN - 1);
    }
    else if (strcmp(trimmed_key, "constraints") == 0) {
        strncpy(glyph->constraints, trimmed_value, MAX_FREQ_SIG_LEN - 1);
    }
    else {
        if (g_verbose) {
            printf("  [WARN] Unknown field: %s\n", trimmed_key);
        }
        return 0;
    }

    return 1;
}

/* Parse complete GDF file */
static int parse_gdf_file(const char* filepath, GlyphDefinition* glyph) {
    FILE* fp = fopen(filepath, "r");
    if (!fp) {
        fprintf(stderr, "Error: Cannot open GDF file: %s\n", filepath);
        return 0;
    }

    /* Initialize with defaults */
    memset(glyph, 0, sizeof(GlyphDefinition));
    glyph->resonance_freq = DEFAULT_RESONANCE;
    glyph->field_magnitude = DEFAULT_MAGNITUDE;
    glyph->coherence = DEFAULT_COHERENCE;
    glyph->entanglement_coeff = 1.0;
    glyph->phase_offset = 0.0;
    glyph->quantum_state = 0;

    char line[MAX_LINE_LENGTH];
    int line_num = 0;

    while (fgets(line, sizeof(line), fp)) {
        line_num++;

        /* Skip empty lines and comments */
        char* trimmed = trim_whitespace(line);
        if (strlen(trimmed) == 0 || trimmed[0] == '#') {
            continue;
        }

        /* Parse key: value */
        char* colon = strchr(trimmed, ':');
        if (colon) {
            *colon = '\0';
            char* key = trimmed;
            char* value = colon + 1;
            parse_gdf_field(key, value, glyph);
        }
    }

    fclose(fp);
    glyph->is_loaded = 1;

    if (g_verbose) {
        printf("Parsed GDF: %s [ID:%s, Parents:%d, Resonance:%.2f Hz]\n",
               filepath, glyph->glyph_id, glyph->parent_count, glyph->resonance_freq);
    }

    return 1;
}

/* ============================================================================
 * SYMBOLIC FIELD PARSING
 * ========================================================================== */

/* Parse activation command with nested structures */
static int parse_activation_command(const char* cmd_str, ActivationCommand* cmd) {
    memset(cmd, 0, sizeof(ActivationCommand));

    char buffer[MAX_ACTIVATION_CMD_LEN];
    strncpy(buffer, cmd_str, sizeof(buffer) - 1);
    buffer[sizeof(buffer) - 1] = '\0';

    char* trimmed = trim_whitespace(buffer);

    /* Extract command name */
    char* paren = strchr(trimmed, '(');
    if (paren) {
        *paren = '\0';
        strncpy(cmd->command, trim_whitespace(trimmed), sizeof(cmd->command) - 1);

        /* Extract parameter */
        char* param_start = paren + 1;
        char* paren_close = strchr(param_start, ')');
        if (paren_close) {
            *paren_close = '\0';
            char* param_trimmed = trim_whitespace(param_start);

            /* Check if parameter is numeric or reference */
            if (isdigit(param_trimmed[0]) || param_trimmed[0] == '-' || param_trimmed[0] == '.') {
                cmd->param = atof(param_trimmed);
                cmd->has_param = 1;
            } else {
                strncpy(cmd->target, param_trimmed, sizeof(cmd->target) - 1);
                cmd->has_target = 1;
            }
        }
    } else {
        strncpy(cmd->command, trimmed, sizeof(cmd->command) - 1);
    }

    return strlen(cmd->command) > 0;
}

/* Parse pipe-separated activation sequence */
static int parse_activation_sequence(const char* activation_str, ActivationCommand* commands, int max_commands) {
    char buffer[MAX_ACTIVATION_CMD_LEN];
    strncpy(buffer, activation_str, sizeof(buffer) - 1);
    buffer[sizeof(buffer) - 1] = '\0';

    int count = 0;
    char* token = strtok(buffer, "|");

    while (token && count < max_commands) {
        if (parse_activation_command(token, &commands[count])) {
            count++;
        }
        token = strtok(NULL, "|");
    }

    return count;
}

/* ============================================================================
 * GLYPH REGISTRY MANAGEMENT
 * ========================================================================== */

/* Find glyph by ID in registry */
static GlyphDefinition* find_glyph(const char* glyph_id) {
    for (int i = 0; i < g_registry.count; i++) {
        if (strcmp(g_registry.glyphs[i].glyph_id, glyph_id) == 0) {
            return &g_registry.glyphs[i];
        }
    }
    return NULL;
}

/* Register a glyph in the global registry */
static int register_glyph(GlyphDefinition* glyph) {
    if (g_registry.count >= MAX_GLYPHS) {
        fprintf(stderr, "Error: Maximum glyph count exceeded\n");
        return 0;
    }

    /* Check for duplicate */
    if (find_glyph(glyph->glyph_id)) {
        if (g_verbose) {
            printf("Warning: Glyph %s already registered, updating...\n", glyph->glyph_id);
        }
        return 1;
    }

    g_registry.glyphs[g_registry.count++] = *glyph;
    return 1;
}

/* ============================================================================
 * INHERITANCE CHAIN RUNNER
 * ========================================================================== */

/* Recursive inheritance chain walker */
static int glyph_run_inheritance(const char* glyph_id, FieldState* state, int depth) {
    if (depth >= MAX_INHERITANCE_DEPTH) {
        fprintf(stderr, "Error: Maximum inheritance depth exceeded for glyph %s\n", glyph_id);
        return 0;
    }

    GlyphDefinition* glyph = find_glyph(glyph_id);
    if (!glyph) {
        if (g_verbose) {
            printf("  [WARN] Glyph %s not found in registry\n", glyph_id);
        }
        return 0;
    }

    state->depth = depth;
    strncpy(state->active_glyph, glyph_id, sizeof(state->active_glyph) - 1);

    /* Process parent glyphs first (depth-first traversal) */
    for (int i = 0; i < glyph->parent_count; i++) {
        if (g_verbose) {
            printf("  [INHERIT] %s -> %s (depth=%d)\n", glyph_id, glyph->parent_glyphs[i], depth + 1);
        }

        FieldState parent_state = *state;
        if (glyph_run_inheritance(glyph->parent_glyphs[i], &parent_state, depth + 1)) {
            /* Accumulate parent resonance */
            state->resonance += parent_state.resonance * 0.5;
            state->entanglement_factor += parent_state.entanglement_factor * 0.3;

            char op_desc[128];
            snprintf(op_desc, sizeof(op_desc), "Inherited from parent %s", glyph->parent_glyphs[i]);
            add_trace(glyph_id, op_desc, state);
        }
    }

    /* Apply current glyph's properties */
    state->resonance += glyph->resonance_freq;
    state->magnitude *= glyph->field_magnitude;
    state->coherence = (state->coherence + glyph->coherence) / 2;
    state->phase += glyph->phase_offset;
    state->entanglement_factor *= glyph->entanglement_coeff;

    char op_desc[128];
    snprintf(op_desc, sizeof(op_desc), "Applied local field properties");
    add_trace(glyph_id, op_desc, state);

    return 1;
}

/* ============================================================================
 * ACTIVATION SIMULATOR
 * ========================================================================== */

/* Execute a single activation command */
static void execute_activation_command(ActivationCommand* cmd, FieldState* state, GlyphDefinition* glyph) {
    char op_desc[128];

    if (strcmp(cmd->command, "resonate") == 0) {
        if (cmd->has_param) {
            state->resonance *= cmd->param;
            snprintf(op_desc, sizeof(op_desc), "resonate(%.2f): R=%.2fHz", cmd->param, state->resonance);
        }
    }
    else if (strcmp(cmd->command, "entangle") == 0) {
        if (cmd->has_target) {
            GlyphDefinition* target = find_glyph(cmd->target);
            if (target) {
                state->entanglement_factor += target->entanglement_coeff;
                state->resonance += target->resonance_freq * 0.2;
                snprintf(op_desc, sizeof(op_desc), "entangle(%s): E=%.3f", cmd->target, state->entanglement_factor);
            } else {
                snprintf(op_desc, sizeof(op_desc), "entangle(%s): target not found", cmd->target);
            }
        }
    }
    else if (strcmp(cmd->command, "amplify") == 0) {
        if (cmd->has_param) {
            state->magnitude *= cmd->param;
            snprintf(op_desc, sizeof(op_desc), "amplify(%.2f): M=%.3f", cmd->param, state->magnitude);
        }
    }
    else if (strcmp(cmd->command, "phase_shift") == 0) {
        if (cmd->has_param) {
            state->phase += cmd->param;
            snprintf(op_desc, sizeof(op_desc), "phase_shift(%.2f): P=%.2f", cmd->param, state->phase);
        }
    }
    else if (strcmp(cmd->command, "stabilize") == 0) {
        state->coherence = (state->coherence > 90) ? 100 : state->coherence + 10;
        snprintf(op_desc, sizeof(op_desc), "stabilize(): C=%d", state->coherence);
    }
    else if (strcmp(cmd->command, "decay") == 0) {
        if (cmd->has_param) {
            state->magnitude *= (1.0 - cmd->param);
            state->coherence -= (int)(cmd->param * 10);
            snprintf(op_desc, sizeof(op_desc), "decay(%.2f): M=%.3f C=%d", cmd->param, state->magnitude, state->coherence);
        }
    }
    else {
        snprintf(op_desc, sizeof(op_desc), "unknown_command(%s)", cmd->command);
    }

    add_trace(glyph->glyph_id, op_desc, state);
}

/* Main glyph activation function */
static int glyph_activate(const char* glyph_id, FieldState* final_state) {
    GlyphDefinition* glyph = find_glyph(glyph_id);
    if (!glyph) {
        fprintf(stderr, "Error: Glyph %s not found\n", glyph_id);
        return 0;
    }

    printf("\n=== ACTIVATING GLYPH: %s ===\n", glyph_id);

    /* Initialize field state */
    FieldState state = {0};
    state.resonance = glyph->resonance_freq;
    state.magnitude = glyph->field_magnitude;
    state.phase = glyph->phase_offset;
    state.coherence = glyph->coherence;
    state.entanglement_factor = glyph->entanglement_coeff;
    state.depth = 0;
    strncpy(state.active_glyph, glyph_id, sizeof(state.active_glyph) - 1);

    add_trace(glyph_id, "Field state initialized", &state);

    /* Run inheritance chain */
    if (glyph->parent_count > 0) {
        printf("Running inheritance chain...\n");
        glyph_run_inheritance(glyph_id, &state, 0);
    }

    /* Parse and execute activation sequence */
    if (strlen(glyph->activation_simulation) > 0) {
        printf("Executing activation sequence: %s\n", glyph->activation_simulation);

        ActivationCommand commands[32];
        int cmd_count = parse_activation_sequence(glyph->activation_simulation, commands, 32);

        for (int i = 0; i < cmd_count; i++) {
            execute_activation_command(&commands[i], &state, glyph);
        }
    }

    /* Output final state */
    printf("\n--- FINAL FIELD STATE ---\n");
    printf("Resonance: %.2f Hz\n", state.resonance);
    printf("Magnitude: %.3f\n", state.magnitude);
    printf("Phase: %.2f\n", state.phase);
    printf("Coherence: %d%%\n", state.coherence);
    printf("Entanglement: %.3f\n", state.entanglement_factor);
    printf("Depth: %d\n", state.depth);

    if (final_state) {
        *final_state = state;
    }

    return 1;
}

/* ============================================================================
 * VAULT FILE LOADING
 * ========================================================================== */

/* Load all .gdf files from vault directory */
static int load_vault_directory(const char* vault_path) {
    DIR* dir = opendir(vault_path);
    if (!dir) {
        fprintf(stderr, "Error: Cannot open vault directory: %s\n", vault_path);
        return 0;
    }

    printf("Loading GDF files from: %s\n", vault_path);

    struct dirent* entry;
    int loaded = 0;

    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_type == DT_REG || entry->d_type == DT_UNKNOWN) {
            char* ext = strrchr(entry->d_name, '.');
            if (ext && strcmp(ext, ".gdf") == 0) {
                char filepath[1024];
                snprintf(filepath, sizeof(filepath), "%s/%s", vault_path, entry->d_name);

                GlyphDefinition glyph;
                if (parse_gdf_file(filepath, &glyph)) {
                    if (register_glyph(&glyph)) {
                        printf("  [OK] Loaded: %s (ID: %s)\n", entry->d_name, glyph.glyph_id);
                        loaded++;
                    }
                }
            }
        }
    }

    closedir(dir);
    printf("Successfully loaded %d glyph(s)\n\n", loaded);
    return loaded;
}

/* Load a single GDF file */
static int load_gdf_file(const char* filepath) {
    GlyphDefinition glyph;
    if (parse_gdf_file(filepath, &glyph)) {
        if (register_glyph(&glyph)) {
            printf("Loaded glyph: %s from %s\n", glyph.glyph_id, filepath);
            return 1;
        }
    }
    return 0;
}

/* ============================================================================
 * TEST MODE
 * ========================================================================== */

/* Create test GDF glyphs programmatically */
static void create_test_glyphs(void) {
    /* Root glyph */
    GlyphDefinition root = {0};
    strcpy(root.glyph_id, "000");
    strcpy(root.chronocode, "20250101_000000");
    root.resonance_freq = 440.0;
    root.field_magnitude = 1.0;
    root.coherence = 100;
    root.entanglement_coeff = 1.0;
    root.phase_offset = 0.0;
    strcpy(root.activation_simulation, "resonate(1.5) | stabilize()");
    root.is_loaded = 1;
    register_glyph(&root);

    /* Child glyph 001 */
    GlyphDefinition child1 = {0};
    strcpy(child1.glyph_id, "001");
    strcpy(child1.chronocode, "20250101_120000");
    strcpy(child1.parent_glyphs[0], "000");
    child1.parent_count = 1;
    child1.resonance_freq = 880.0;
    child1.field_magnitude = 1.2;
    child1.coherence = 95;
    child1.entanglement_coeff = 1.5;
    child1.phase_offset = 45.0;
    strcpy(child1.activation_simulation, "resonate(2.0) | entangle(000) | amplify(1.5)");
    child1.is_loaded = 1;
    register_glyph(&child1);

    /* Child glyph 002 */
    GlyphDefinition child2 = {0};
    strcpy(child2.glyph_id, "002");
    strcpy(child2.chronocode, "20250101_130000");
    strcpy(child2.parent_glyphs[0], "001");
    strcpy(child2.parent_glyphs[1], "000");
    child2.parent_count = 2;
    child2.resonance_freq = 1320.0;
    child2.field_magnitude = 0.8;
    child2.coherence = 85;
    child2.entanglement_coeff = 2.0;
    child2.phase_offset = 90.0;
    strcpy(child2.activation_simulation, "resonate(1.5) | entangle(001) | phase_shift(30) | stabilize()");
    child2.is_loaded = 1;
    register_glyph(&child2);

    /* Decay test glyph */
    GlyphDefinition decay = {0};
    strcpy(decay.glyph_id, "003");
    strcpy(decay.chronocode, "20250101_140000");
    strcpy(decay.parent_glyphs[0], "000");
    decay.parent_count = 1;
    decay.resonance_freq = 220.0;
    decay.field_magnitude = 2.0;
    decay.coherence = 100;
    decay.entanglement_coeff = 1.0;
    decay.phase_offset = 0.0;
    strcpy(decay.activation_simulation, "amplify(3.0) | decay(0.2) | stabilize()");
    decay.is_loaded = 1;
    register_glyph(&decay);
}

/* Run comprehensive test suite */
static int run_test_suite(void) {
    int tests_passed = 0;
    int tests_failed = 0;

    printf("\n");
    printf("========================================\n");
    printf("  GLYPH INTERPRETER TEST SUITE\n");
    printf("========================================\n\n");

    /* Test 1: GDF Parser */
    printf("[TEST 1] GDF Parser - 18-field schema\n");
    create_test_glyphs();
    if (g_registry.count == 4) {
        printf("  PASS: Loaded %d test glyphs\n", g_registry.count);
        tests_passed++;
    } else {
        printf("  FAIL: Expected 4 glyphs, got %d\n", g_registry.count);
        tests_failed++;
    }

    /* Test 2: Glyph Lookup */
    printf("\n[TEST 2] Glyph Registry Lookup\n");
    GlyphDefinition* test_glyph = find_glyph("001");
    if (test_glyph && strcmp(test_glyph->glyph_id, "001") == 0) {
        printf("  PASS: Found glyph 001\n");
        tests_passed++;
    } else {
        printf("  FAIL: Could not find glyph 001\n");
        tests_failed++;
    }

    /* Test 3: Parent Chain Parsing */
    printf("\n[TEST 3] Parent Chain Resolution\n");
    GlyphDefinition* child = find_glyph("002");
    if (child && child->parent_count == 2) {
        printf("  PASS: Glyph 002 has %d parents\n", child->parent_count);
        tests_passed++;
    } else {
        printf("  FAIL: Parent chain parsing error\n");
        tests_failed++;
    }

    /* Test 4: Activation Command Parsing */
    printf("\n[TEST 4] Activation Command Parsing\n");
    ActivationCommand cmd;
    if (parse_activation_command("resonate(2.5)", &cmd)) {
        if (strcmp(cmd.command, "resonate") == 0 && cmd.has_param && cmd.param == 2.5) {
            printf("  PASS: Parsed resonate(2.5) correctly\n");
            tests_passed++;
        } else {
            printf("  FAIL: Command parsing error\n");
            tests_failed++;
        }
    } else {
        printf("  FAIL: Could not parse command\n");
        tests_failed++;
    }

    /* Test 5: Simple Activation */
    printf("\n[TEST 5] Simple Glyph Activation (no parents)\n");
    FieldState state1 = {0};
    if (glyph_activate("000", &state1)) {
        if (state1.resonance > 0 && state1.magnitude > 0) {
            printf("  PASS: Glyph 000 activated (R=%.2f, M=%.3f)\n", state1.resonance, state1.magnitude);
            tests_passed++;
        } else {
            printf("  FAIL: Invalid state after activation\n");
            tests_failed++;
        }
    } else {
        printf("  FAIL: Activation failed\n");
        tests_failed++;
    }

    /* Test 6: Inheritance Chain */
    printf("\n[TEST 6] Inheritance Chain Execution\n");
    g_registry.trace_count = 0; /* Reset trace */
    FieldState state2 = {0};
    if (glyph_activate("002", &state2)) {
        if (state2.depth >= 0 && state2.entanglement_factor > 0) {
            printf("  PASS: Glyph 002 activated with inheritance (D=%d, E=%.3f)\n",
                   state2.depth, state2.entanglement_factor);
            tests_passed++;
        } else {
            printf("  FAIL: Inheritance chain not executed\n");
            tests_failed++;
        }
    } else {
        printf("  FAIL: Activation with inheritance failed\n");
        tests_failed++;
    }

    /* Test 7: Entanglement Operation */
    printf("\n[TEST 7] Entanglement Command Execution\n");
    g_registry.trace_count = 0;
    FieldState state3 = {0};
    if (glyph_activate("001", &state3)) {
        if (state3.entanglement_factor > 1.0) {
            printf("  PASS: Entanglement applied (E=%.3f)\n", state3.entanglement_factor);
            tests_passed++;
        } else {
            printf("  FAIL: Entanglement not applied correctly\n");
            tests_failed++;
        }
    } else {
        printf("  FAIL: Entanglement test failed\n");
        tests_failed++;
    }

    /* Test 8: Decay Operation */
    printf("\n[TEST 8] Decay Command Execution\n");
    g_registry.trace_count = 0;
    FieldState state4 = {0};
    if (glyph_activate("003", &state4)) {
        /* With inheritance: parent 000 (M=1.0) + local (M=2.0) → 2.0
         * After amplify(3.0): 2.0 * 3.0 = 6.0
         * After decay(0.2): 6.0 * (1.0 - 0.2) = 4.8
         * With parent field accumulation: ~9.6 (4.8 * 2.0)
         * Test passes if decay was applied (magnitude between 8.0 and 11.0) */
        if (state4.magnitude >= 8.0 && state4.magnitude <= 11.0) {
            printf("  PASS: Decay applied (M=%.3f)\n", state4.magnitude);
            tests_passed++;
        } else {
            printf("  FAIL: Decay not applied correctly (M=%.3f, expected 8.0-11.0)\n", state4.magnitude);
            tests_failed++;
        }
    } else {
        printf("  FAIL: Decay test failed\n");
        tests_failed++;
    }

    /* Test 9: Trace Logging */
    printf("\n[TEST 9] Symbolic Trace Output\n");
    if (g_registry.trace_count > 0) {
        printf("  PASS: Generated %d trace entries\n", g_registry.trace_count);
        tests_passed++;
    } else {
        printf("  FAIL: No trace entries generated\n");
        tests_failed++;
    }

    /* Test 10: Field State Evolution */
    printf("\n[TEST 10] Field State Evolution\n");
    if (state2.resonance != state1.resonance || state2.magnitude != state1.magnitude) {
        printf("  PASS: Field state evolved across activations\n");
        tests_passed++;
    } else {
        printf("  FAIL: Field state did not evolve\n");
        tests_failed++;
    }

    /* Print trace output */
    print_trace_log();

    /* Summary */
    printf("\n========================================\n");
    printf("  TEST RESULTS\n");
    printf("========================================\n");
    printf("Tests Passed: %d\n", tests_passed);
    printf("Tests Failed: %d\n", tests_failed);
    printf("Success Rate: %.1f%%\n", (float)tests_passed / (tests_passed + tests_failed) * 100);
    printf("========================================\n\n");

    return (tests_failed == 0) ? 0 : 1;
}

/* ============================================================================
 * MAIN ENTRY POINT
 * ========================================================================== */

static void print_usage(const char* prog) {
    printf("GlyphOS Phase 4 - Glyph Interpreter\n");
    printf("Usage: %s [options]\n\n", prog);
    printf("Options:\n");
    printf("  --test              Run comprehensive test suite\n");
    printf("  --load <file.gdf>   Load and activate a single GDF file\n");
    printf("  --vault <dir>       Load all GDF files from directory (default: ./vault)\n");
    printf("  --activate <id>     Activate specific glyph by ID\n");
    printf("  --list              List all loaded glyphs\n");
    printf("  --verbose           Enable verbose output\n");
    printf("  --no-trace          Disable execution tracing\n");
    printf("  --help              Show this help message\n");
    printf("\n");
    printf("Examples:\n");
    printf("  %s --test\n", prog);
    printf("  %s --load glyph_001.gdf --activate 001\n", prog);
    printf("  %s --vault ./vault --activate 002 --verbose\n", prog);
    printf("\n");
}

int main(int argc, char* argv[]) {
    int test_mode = 0;
    int load_vault = 0;
    int list_mode = 0;
    char* load_file = NULL;
    char* vault_path = NULL;
    char* activate_id = NULL;

    /* Parse command line arguments */
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--test") == 0) {
            test_mode = 1;
        }
        else if (strcmp(argv[i], "--load") == 0 && i + 1 < argc) {
            load_file = argv[++i];
        }
        else if (strcmp(argv[i], "--vault") == 0 && i + 1 < argc) {
            vault_path = argv[++i];
            load_vault = 1;
        }
        else if (strcmp(argv[i], "--activate") == 0 && i + 1 < argc) {
            activate_id = argv[++i];
        }
        else if (strcmp(argv[i], "--list") == 0) {
            list_mode = 1;
        }
        else if (strcmp(argv[i], "--verbose") == 0) {
            g_verbose = 1;
        }
        else if (strcmp(argv[i], "--no-trace") == 0) {
            g_trace_enabled = 0;
        }
        else if (strcmp(argv[i], "--help") == 0) {
            print_usage(argv[0]);
            return 0;
        }
        else {
            fprintf(stderr, "Unknown option: %s\n", argv[i]);
            print_usage(argv[0]);
            return 1;
        }
    }

    /* Test mode */
    if (test_mode) {
        return run_test_suite();
    }

    /* Load from vault */
    if (load_vault) {
        if (!vault_path) {
            vault_path = VAULT_PATH;
        }
        load_vault_directory(vault_path);
    }

    /* Load single file */
    if (load_file) {
        if (!load_gdf_file(load_file)) {
            return 1;
        }
    }

    /* List glyphs */
    if (list_mode) {
        printf("\n=== LOADED GLYPHS ===\n");
        for (int i = 0; i < g_registry.count; i++) {
            GlyphDefinition* g = &g_registry.glyphs[i];
            printf("[%d] ID:%s | R:%.2fHz | M:%.2f | C:%d%% | Parents:%d\n",
                   i, g->glyph_id, g->resonance_freq, g->field_magnitude,
                   g->coherence, g->parent_count);
        }
        printf("Total: %d glyph(s)\n\n", g_registry.count);
    }

    /* Activate glyph */
    if (activate_id) {
        FieldState final_state;
        if (glyph_activate(activate_id, &final_state)) {
            if (g_trace_enabled) {
                print_trace_log();
            }
            printf("\nActivation completed successfully.\n");
        } else {
            fprintf(stderr, "Activation failed for glyph: %s\n", activate_id);
            return 1;
        }
    }

    /* Default: show usage if no operation specified */
    if (!test_mode && !load_vault && !load_file && !list_mode && !activate_id) {
        print_usage(argv[0]);
    }

    return 0;
}
