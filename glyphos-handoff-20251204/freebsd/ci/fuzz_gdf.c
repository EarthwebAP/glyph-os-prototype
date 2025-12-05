/*
 * GlyphOS GDF Fuzzer
 * Fuzzing harness for GDF parser using libFuzzer or AFL
 *
 * Build with libFuzzer:
 *   clang -fsanitize=fuzzer,address -g -O1 ci/fuzz_gdf.c -o bin/fuzz_gdf -lm
 *
 * Build with AFL:
 *   afl-clang-fast -fsanitize=address -g -O1 ci/fuzz_gdf.c -o bin/fuzz_gdf_afl -lm
 *
 * Run:
 *   mkdir -p corpus
 *   cp vault/*.gdf corpus/
 *   ./bin/fuzz_gdf corpus/ -max_total_time=3600
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

/* Minimal GDF parser structures (copy from glyph_interpreter.c) */
#define MAX_GLYPH_ID_LEN 64
#define MAX_CHRONOCODE_LEN 32
#define MAX_PARENT_GLYPHS 16
#define MAX_FIELD_LEN 256
#define MAX_LINE_LENGTH 2048

typedef struct {
    char glyph_id[MAX_GLYPH_ID_LEN];
    char chronocode[MAX_CHRONOCODE_LEN];
    char parent_glyphs[MAX_PARENT_GLYPHS][MAX_GLYPH_ID_LEN];
    int parent_count;
    double resonance_freq;
    double field_magnitude;
    int coherence;
    char material_spec[MAX_FIELD_LEN];
    char activation_script[MAX_FIELD_LEN];
} GlyphDef;

/* Trim whitespace */
static char* trim_whitespace(char* str) {
    char* end;
    while(*str == ' ' || *str == '\t' || *str == '\n' || *str == '\r') str++;
    if(*str == 0) return str;
    end = str + strlen(str) - 1;
    while(end > str && (*end == ' ' || *end == '\t' || *end == '\n' || *end == '\r')) end--;
    end[1] = '\0';
    return str;
}

/* Parse GDF field */
static int parse_gdf_field(const char* key, const char* value, GlyphDef* glyph) {
    char* trimmed_key = trim_whitespace((char*)key);
    char* trimmed_value = trim_whitespace((char*)value);

    if (strcmp(trimmed_key, "glyph_id") == 0) {
        strncpy(glyph->glyph_id, trimmed_value, MAX_GLYPH_ID_LEN - 1);
        glyph->glyph_id[MAX_GLYPH_ID_LEN - 1] = '\0';
    } else if (strcmp(trimmed_key, "chronocode") == 0) {
        strncpy(glyph->chronocode, trimmed_value, MAX_CHRONOCODE_LEN - 1);
        glyph->chronocode[MAX_CHRONOCODE_LEN - 1] = '\0';
    } else if (strcmp(trimmed_key, "resonance_freq") == 0 || strcmp(trimmed_key, "resonance") == 0) {
        glyph->resonance_freq = atof(trimmed_value);
    } else if (strcmp(trimmed_key, "field_magnitude") == 0 || strcmp(trimmed_key, "magnitude") == 0) {
        glyph->field_magnitude = atof(trimmed_value);
    } else if (strcmp(trimmed_key, "coherence") == 0) {
        glyph->coherence = atoi(trimmed_value);
    } else if (strcmp(trimmed_key, "material_spec") == 0 || strcmp(trimmed_key, "material") == 0) {
        strncpy(glyph->material_spec, trimmed_value, MAX_FIELD_LEN - 1);
        glyph->material_spec[MAX_FIELD_LEN - 1] = '\0';
    } else if (strcmp(trimmed_key, "activation_simulation") == 0 || strcmp(trimmed_key, "activation") == 0) {
        strncpy(glyph->activation_script, trimmed_value, MAX_FIELD_LEN - 1);
        glyph->activation_script[MAX_FIELD_LEN - 1] = '\0';
    }

    return 0;
}

/* Parse GDF from buffer */
static int parse_gdf_buffer(const uint8_t* data, size_t size) {
    if (size == 0 || size > 1024 * 1024) {
        return -1;  /* Reject empty or too large inputs */
    }

    /* Copy to null-terminated buffer */
    char* buffer = malloc(size + 1);
    if (!buffer) return -1;
    memcpy(buffer, data, size);
    buffer[size] = '\0';

    GlyphDef glyph;
    memset(&glyph, 0, sizeof(GlyphDef));

    /* Parse line by line */
    char* line = buffer;
    char* next_line;

    while (line && *line) {
        /* Find next line */
        next_line = strchr(line, '\n');
        if (next_line) {
            *next_line = '\0';
            next_line++;
        }

        /* Trim line */
        char* trimmed = trim_whitespace(line);

        /* Skip empty lines and comments */
        if (strlen(trimmed) == 0 || trimmed[0] == '#') {
            line = next_line;
            continue;
        }

        /* Parse key: value */
        char* colon = strchr(trimmed, ':');
        if (colon) {
            *colon = '\0';
            char* key = trimmed;
            char* value = colon + 1;
            parse_gdf_field(key, value, &glyph);
        }

        line = next_line;
    }

    free(buffer);

    /* Validate glyph */
    if (strlen(glyph.glyph_id) == 0) {
        return -1;  /* Invalid: no glyph_id */
    }

    if (glyph.resonance_freq < 0 || glyph.resonance_freq > 100000) {
        return -1;  /* Invalid: resonance out of range */
    }

    if (glyph.field_magnitude < 0 || glyph.field_magnitude > 1000) {
        return -1;  /* Invalid: magnitude out of range */
    }

    if (glyph.coherence < 0 || glyph.coherence > 100) {
        return -1;  /* Invalid: coherence out of range */
    }

    return 0;
}

/* libFuzzer entry point */
int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
    parse_gdf_buffer(data, size);
    return 0;
}

/* AFL entry point (when not using libFuzzer) */
#ifndef __AFL_FUZZ_TESTCASE_BUF
int main(int argc, char** argv) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <input_file>\n", argv[0]);
        return 1;
    }

    FILE* fp = fopen(argv[1], "rb");
    if (!fp) {
        perror("fopen");
        return 1;
    }

    fseek(fp, 0, SEEK_END);
    long size = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    uint8_t* buffer = malloc(size);
    if (!buffer) {
        fclose(fp);
        return 1;
    }

    fread(buffer, 1, size, fp);
    fclose(fp);

    int result = parse_gdf_buffer(buffer, size);
    free(buffer);

    return result;
}
#endif
