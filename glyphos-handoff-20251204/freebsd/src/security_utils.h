/*
 * GlyphOS Security Utilities
 *
 * Common security functions for path validation, input sanitization,
 * and cycle detection.
 */

#ifndef SECURITY_UTILS_H
#define SECURITY_UTILS_H

#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/* Path validation */
int validate_vault_path(const char* vault_path, const char* filename, char* resolved_out, size_t out_size);
int is_safe_filename(const char* filename);

/* String sanitization */
int sanitize_string(char* str, size_t max_len);
int validate_glyph_id(const char* glyph_id);

/* Numeric validation */
int validate_range_double(double value, double min, double max, const char* field_name);
int validate_range_int(int value, int min, int max, const char* field_name);

/* Cycle detection for inheritance */
typedef struct {
    const char* visited[32];  /* MAX_INHERITANCE_DEPTH */
    int count;
} InheritanceContext;

int inheritance_context_init(InheritanceContext* ctx);
int inheritance_context_visit(InheritanceContext* ctx, const char* glyph_id);
int inheritance_context_is_visited(const InheritanceContext* ctx, const char* glyph_id);

#endif /* SECURITY_UTILS_H */
