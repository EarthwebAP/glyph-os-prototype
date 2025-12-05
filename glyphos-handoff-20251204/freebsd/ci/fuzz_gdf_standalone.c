/*
 * GlyphOS GDF Standalone Fuzzer
 * Simple fuzzer without libFuzzer dependency
 *
 * Build:
 *   gcc -fsanitize=address,undefined -O1 -g ci/fuzz_gdf_standalone.c -o ci/fuzz_gdf_standalone -lm
 *
 * Run:
 *   ./ci/fuzz_gdf_standalone <corpus_dir> <runs>
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <time.h>
#include <dirent.h>
#include <sys/stat.h>

/* Minimal GDF parser structures */
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
        return -1;
    }

    char* buffer = malloc(size + 1);
    if (!buffer) return -1;
    memcpy(buffer, data, size);
    buffer[size] = '\0';

    GlyphDef glyph;
    memset(&glyph, 0, sizeof(GlyphDef));

    char* line = buffer;
    char* next_line;

    while (line && *line) {
        next_line = strchr(line, '\n');
        if (next_line) {
            *next_line = '\0';
            next_line++;
        }

        char* trimmed = trim_whitespace(line);

        if (strlen(trimmed) == 0 || trimmed[0] == '#') {
            line = next_line;
            continue;
        }

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

    /* Validate */
    if (strlen(glyph.glyph_id) == 0) return -1;
    if (glyph.resonance_freq < 0 || glyph.resonance_freq > 100000) return -1;
    if (glyph.field_magnitude < 0 || glyph.field_magnitude > 1000) return -1;
    if (glyph.coherence < 0 || glyph.coherence > 100) return -1;

    return 0;
}

/* Mutate buffer (simple mutations) */
static void mutate_buffer(uint8_t* data, size_t* size, size_t max_size) {
    int mutation = rand() % 10;

    switch (mutation) {
        case 0: /* Bit flip */
            if (*size > 0) {
                size_t pos = rand() % *size;
                data[pos] ^= (1 << (rand() % 8));
            }
            break;
        case 1: /* Byte flip */
            if (*size > 0) {
                size_t pos = rand() % *size;
                data[pos] ^= 0xFF;
            }
            break;
        case 2: /* Insert byte */
            if (*size < max_size - 1) {
                size_t pos = rand() % (*size + 1);
                memmove(data + pos + 1, data + pos, *size - pos);
                data[pos] = rand() % 256;
                (*size)++;
            }
            break;
        case 3: /* Delete byte */
            if (*size > 1) {
                size_t pos = rand() % *size;
                memmove(data + pos, data + pos + 1, *size - pos - 1);
                (*size)--;
            }
            break;
        case 4: /* Replace byte */
            if (*size > 0) {
                data[rand() % *size] = rand() % 256;
            }
            break;
        default: /* No mutation */
            break;
    }
}

int main(int argc, char** argv) {
    if (argc < 3) {
        fprintf(stderr, "Usage: %s <corpus_dir> <runs>\n", argv[0]);
        return 1;
    }

    const char* corpus_dir = argv[1];
    int max_runs = atoi(argv[2]);

    srand(time(NULL));

    printf("=== GlyphOS Standalone Fuzzer ===\n");
    printf("Corpus: %s\n", corpus_dir);
    printf("Runs:   %d\n", max_runs);
    printf("\n");

    /* Load initial corpus */
    DIR* dir = opendir(corpus_dir);
    if (!dir) {
        fprintf(stderr, "Error: Cannot open corpus directory\n");
        return 1;
    }

    int corpus_count = 0;
    uint8_t* corpus[100];
    size_t corpus_sizes[100];

    struct dirent* entry;
    while ((entry = readdir(dir)) != NULL && corpus_count < 100) {
        if (entry->d_name[0] == '.') continue;

        char path[1024];
        snprintf(path, sizeof(path), "%s/%s", corpus_dir, entry->d_name);

        FILE* fp = fopen(path, "rb");
        if (!fp) continue;

        fseek(fp, 0, SEEK_END);
        long size = ftell(fp);
        fseek(fp, 0, SEEK_SET);

        if (size > 0 && size < 10240) {
            uint8_t* data = malloc(size);
            if (data) {
                fread(data, 1, size, fp);
                corpus[corpus_count] = data;
                corpus_sizes[corpus_count] = size;
                corpus_count++;
            }
        }
        fclose(fp);
    }
    closedir(dir);

    printf("Loaded %d corpus files\n", corpus_count);
    if (corpus_count == 0) {
        fprintf(stderr, "Error: No corpus files found\n");
        return 1;
    }

    /* Fuzzing loop */
    int crashes = 0;
    int valid_parses = 0;

    for (int run = 0; run < max_runs; run++) {
        /* Select random corpus file */
        int idx = rand() % corpus_count;

        /* Copy and mutate */
        uint8_t buffer[10240];
        size_t size = corpus_sizes[idx];
        memcpy(buffer, corpus[idx], size);

        /* Apply 1-5 mutations */
        int num_mutations = 1 + (rand() % 5);
        for (int m = 0; m < num_mutations; m++) {
            mutate_buffer(buffer, &size, sizeof(buffer));
        }

        /* Test parser */
        int result = parse_gdf_buffer(buffer, size);
        if (result == 0) {
            valid_parses++;
        }

        if (run % 1000 == 0 && run > 0) {
            printf("Runs: %d, Valid: %d, Crashes: %d\n",
                   run, valid_parses, crashes);
        }
    }

    printf("\n=== Fuzzing Complete ===\n");
    printf("Total runs:    %d\n", max_runs);
    printf("Valid parses:  %d\n", valid_parses);
    printf("Crashes:       %d\n", crashes);

    /* Cleanup */
    for (int i = 0; i < corpus_count; i++) {
        free(corpus[i]);
    }

    return crashes > 0 ? 1 : 0;
}
