/*
 * GlyphOS Security Utilities Implementation
 */

#include "security_utils.h"
#include <stdio.h>
#include <ctype.h>
#include <errno.h>

/**
 * Validate that a file path is within the vault directory and doesn't
 * attempt path traversal attacks.
 *
 * Returns: 0 on success, -1 on validation failure
 */
int validate_vault_path(const char* vault_path, const char* filename, char* resolved_out, size_t out_size) {
    char vault_resolved[PATH_MAX];
    char full_path[PATH_MAX];
    char file_resolved[PATH_MAX];

    /* Check for NULL inputs */
    if (!vault_path || !filename || !resolved_out) {
        fprintf(stderr, "Security: NULL input to validate_vault_path\n");
        return -1;
    }

    /* Check filename for suspicious characters */
    if (strstr(filename, "..") != NULL ||
        filename[0] == '/' ||
        strstr(filename, "//") != NULL) {
        fprintf(stderr, "Security: Path traversal attempt detected in: %s\n", filename);
        return -1;
    }

    /* Resolve vault path to absolute path */
    if (realpath(vault_path, vault_resolved) == NULL) {
        fprintf(stderr, "Security: Cannot resolve vault path: %s (%s)\n",
                vault_path, strerror(errno));
        return -1;
    }

    /* Construct full file path */
    if (snprintf(full_path, sizeof(full_path), "%s/%s", vault_resolved, filename) >= sizeof(full_path)) {
        fprintf(stderr, "Security: Path too long\n");
        return -1;
    }

    /* Resolve file path (may not exist yet, that's OK) */
    /* For files that don't exist, validate the directory component */
    char *last_slash = strrchr(full_path, '/');
    if (last_slash) {
        *last_slash = '\0';
        if (realpath(full_path, file_resolved) == NULL) {
            fprintf(stderr, "Security: Cannot resolve file directory: %s\n", full_path);
            return -1;
        }
        *last_slash = '/';
    }

    /* For existing files, get full resolved path */
    if (access(full_path, F_OK) == 0) {
        if (realpath(full_path, file_resolved) == NULL) {
            fprintf(stderr, "Security: Cannot resolve file path: %s\n", full_path);
            return -1;
        }
    } else {
        /* File doesn't exist, use constructed path */
        strncpy(file_resolved, full_path, sizeof(file_resolved) - 1);
        file_resolved[sizeof(file_resolved) - 1] = '\0';
    }

    /* Verify file path starts with vault path (no escape) */
    size_t vault_len = strlen(vault_resolved);
    if (strncmp(file_resolved, vault_resolved, vault_len) != 0 ||
        (file_resolved[vault_len] != '/' && file_resolved[vault_len] != '\0')) {
        fprintf(stderr, "Security: Path escape attempt: %s is not in %s\n",
                file_resolved, vault_resolved);
        return -1;
    }

    /* Copy resolved path to output */
    strncpy(resolved_out, file_resolved, out_size - 1);
    resolved_out[out_size - 1] = '\0';

    return 0;
}

/**
 * Check if filename is safe (no path components, valid characters)
 */
int is_safe_filename(const char* filename) {
    if (!filename || strlen(filename) == 0) {
        return 0;
    }

    /* Check for path separators */
    if (strchr(filename, '/') || strchr(filename, '\\')) {
        return 0;
    }

    /* Check for hidden files (optional - may be legitimate) */
    if (filename[0] == '.') {
        return 0;
    }

    /* Check for control characters */
    for (const char* p = filename; *p; p++) {
        if (iscntrl(*p)) {
            return 0;
        }
    }

    return 1;
}

/**
 * Sanitize string by removing/replacing dangerous characters
 */
int sanitize_string(char* str, size_t max_len) {
    if (!str) {
        return -1;
    }

    size_t len = strnlen(str, max_len);

    for (size_t i = 0; i < len; i++) {
        /* Remove control characters except newline/tab */
        if (iscntrl(str[i]) && str[i] != '\n' && str[i] != '\t') {
            str[i] = ' ';
        }

        /* Remove null bytes in middle of string */
        if (str[i] == '\0' && i < len - 1) {
            str[i] = ' ';
        }
    }

    /* Ensure null termination */
    if (len >= max_len) {
        str[max_len - 1] = '\0';
    }

    return 0;
}

/**
 * Validate glyph ID format
 */
int validate_glyph_id(const char* glyph_id) {
    if (!glyph_id) {
        return 0;
    }

    size_t len = strlen(glyph_id);

    /* Check length (must be reasonable) */
    if (len == 0 || len > 64) {
        return 0;
    }

    /* Check characters (alphanumeric + underscore/hyphen) */
    for (size_t i = 0; i < len; i++) {
        if (!isalnum(glyph_id[i]) && glyph_id[i] != '_' && glyph_id[i] != '-') {
            return 0;
        }
    }

    return 1;
}

/**
 * Validate numeric range with error reporting
 */
int validate_range_double(double value, double min, double max, const char* field_name) {
    /* Check for NaN/Inf */
    if (value != value) {  /* NaN check */
        fprintf(stderr, "Validation error: %s is NaN\n", field_name);
        return -1;
    }

    if (value == 1.0/0.0 || value == -1.0/0.0) {  /* Inf check */
        fprintf(stderr, "Validation error: %s is infinite\n", field_name);
        return -1;
    }

    /* Check range */
    if (value < min || value > max) {
        fprintf(stderr, "Validation error: %s=%.6f is out of range [%.6f, %.6f]\n",
                field_name, value, min, max);
        return -1;
    }

    return 0;
}

/**
 * Validate integer range
 */
int validate_range_int(int value, int min, int max, const char* field_name) {
    if (value < min || value > max) {
        fprintf(stderr, "Validation error: %s=%d is out of range [%d, %d]\n",
                field_name, value, min, max);
        return -1;
    }

    return 0;
}

/**
 * Initialize inheritance context for cycle detection
 */
int inheritance_context_init(InheritanceContext* ctx) {
    if (!ctx) {
        return -1;
    }

    ctx->count = 0;
    memset(ctx->visited, 0, sizeof(ctx->visited));

    return 0;
}

/**
 * Mark a glyph as visited in the inheritance chain
 */
int inheritance_context_visit(InheritanceContext* ctx, const char* glyph_id) {
    if (!ctx || !glyph_id) {
        return -1;
    }

    /* Check if already visited (cycle detection) */
    if (inheritance_context_is_visited(ctx, glyph_id)) {
        fprintf(stderr, "Security: Circular inheritance detected involving %s\n", glyph_id);
        return -1;
    }

    /* Check capacity */
    if (ctx->count >= 32) {
        fprintf(stderr, "Security: Inheritance depth limit exceeded\n");
        return -1;
    }

    /* Add to visited list */
    ctx->visited[ctx->count++] = glyph_id;

    return 0;
}

/**
 * Check if glyph was already visited (cycle detection)
 */
int inheritance_context_is_visited(const InheritanceContext* ctx, const char* glyph_id) {
    if (!ctx || !glyph_id) {
        return 0;
    }

    for (int i = 0; i < ctx->count; i++) {
        if (strcmp(ctx->visited[i], glyph_id) == 0) {
            return 1;  /* Already visited */
        }
    }

    return 0;  /* Not visited */
}
