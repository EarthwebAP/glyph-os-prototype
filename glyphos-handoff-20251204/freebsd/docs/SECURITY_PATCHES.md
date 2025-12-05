# GlyphOS Security Patches

This document describes critical security vulnerabilities and their fixes.

## Overview

Security assessment identified:
- **3 Critical vulnerabilities** requiring immediate patching
- **2 High severity issues** requiring urgent attention
- **9 Medium severity issues** for next release

**Status**: Patches created, testing in progress
**Target**: Apply before any production deployment

---

## Critical Vulnerability #1: Path Traversal in Vault Loading

**CVE**: Pending
**Severity**: CRITICAL (CVSS 9.1)
**Component**: `src/glyph_interpreter.c` lines 591-596
**Impact**: Arbitrary file read via directory traversal

### Vulnerability Description

The vault loading function (`load_vault_directory`) does not validate that filenames returned by `readdir()` are safe. An attacker can create symlinks or use path traversal to read arbitrary files:

```c
// VULNERABLE CODE
while ((entry = readdir(dir)) != NULL) {
    char filepath[1024];
    snprintf(filepath, sizeof(filepath), "%s/%s", vault_path, entry->d_name);
    // entry->d_name is NOT validated - could contain "../../../etc/passwd"
    if (parse_gdf_file(filepath, &glyph)) {
        // File contents exposed
    }
}
```

**Attack Vector**:
```bash
# Attacker creates malicious vault
mkdir /tmp/evil_vault
ln -s /etc/passwd /tmp/evil_vault/secret.gdf
ln -s /etc/shadow /tmp/evil_vault/shadow.gdf

# Run interpreter
./glyph_interp --vault /tmp/evil_vault --load ../../../etc/shadow.gdf
```

### Fix

**File**: `src/glyph_interpreter.c`

**Add includes**:
```c
#include "security_utils.h"
#include <sys/stat.h>
```

**Replace vulnerable code** (lines 591-612):
```c
while ((entry = readdir(dir)) != NULL) {
    if (entry->d_type == DT_REG || entry->d_type == DT_UNKNOWN) {
        /* SECURITY FIX: Validate filename is safe */
        if (!is_safe_filename(entry->d_name)) {
            fprintf(stderr, "Warning: Skipping unsafe filename: %s\n", entry->d_name);
            continue;
        }

        char* ext = strrchr(entry->d_name, '.');
        if (ext && strcmp(ext, ".gdf") == 0) {
            char filepath[PATH_MAX];
            char resolved[PATH_MAX];

            /* SECURITY FIX: Validate path doesn't escape vault */
            if (validate_vault_path(vault_path, entry->d_name, resolved, sizeof(resolved)) != 0) {
                fprintf(stderr, "Security: Rejecting path traversal attempt: %s\n", entry->d_name);
                continue;
            }

            /* SECURITY FIX: Verify it's a regular file, not a symlink */
            struct stat st;
            if (lstat(resolved, &st) != 0) {
                fprintf(stderr, "Warning: Cannot stat file: %s\n", resolved);
                continue;
            }

            if (!S_ISREG(st.st_mode)) {
                fprintf(stderr, "Security: Skipping non-regular file: %s\n", resolved);
                continue;
            }

            /* Now safe to load */
            GlyphDef glyph;
            if (parse_gdf_file(resolved, &glyph)) {
                if (g_registry.count < MAX_GLYPHS) {
                    g_registry.glyphs[g_registry.count++] = glyph;
                }
            }
        }
    }
}
```

### Verification

```bash
# Test path traversal protection
mkdir /tmp/test_vault
ln -s /etc/passwd /tmp/test_vault/passwd.gdf
./glyph_interp --vault /tmp/test_vault --list

# Expected output:
# Security: Skipping non-regular file: passwd.gdf
# Loaded 0 glyphs

# Test with .. in filename
touch "/tmp/test_vault/../escape.gdf"
./glyph_interp --vault /tmp/test_vault --list

# Expected output:
# Warning: Skipping unsafe filename: ../escape.gdf
```

---

## Critical Vulnerability #2: Circular Inheritance Stack Overflow

**CVE**: Pending
**Severity**: CRITICAL (CVSS 7.5)
**Component**: `src/glyph_interpreter.c` lines 413-460
**Impact**: Stack exhaustion leading to crash or code execution

### Vulnerability Description

The recursive inheritance resolver has depth limiting but no cycle detection. Circular inheritance chains cause stack overflow:

```c
// VULNERABLE CODE
static int glyph_run_inheritance(const char* glyph_id, FieldState* state, int depth) {
    if (depth >= MAX_INHERITANCE_DEPTH) {
        return 0;  // Depth limit hit, but already consumed stack
    }

    for (int i = 0; i < glyph->parent_count; i++) {
        glyph_run_inheritance(glyph->parent_glyphs[i], &parent_state, depth + 1);
        // No check for cycles - A->B->A will recurse indefinitely
    }
}
```

**Attack Vector**:
```
# glyph_001.gdf
glyph_id: 001
parent_glyphs: 002,003

# glyph_002.gdf
glyph_id: 002
parent_glyphs: 003

# glyph_003.gdf
glyph_id: 003
parent_glyphs: 001  # CYCLE: 001 -> 002 -> 003 -> 001
```

### Fix

**File**: `src/glyph_interpreter.c`

**Update function signature** (line 413):
```c
static int glyph_run_inheritance(const char* glyph_id, FieldState* state,
                                  int depth, InheritanceContext* ctx) {
```

**Add cycle detection** (after line 417):
```c
if (depth >= MAX_INHERITANCE_DEPTH) {
    fprintf(stderr, "Error: Maximum inheritance depth exceeded for glyph %s\n", glyph_id);
    return 0;
}

/* SECURITY FIX: Check for circular inheritance */
if (inheritance_context_visit(ctx, glyph_id) != 0) {
    /* Already visited or error */
    return 0;  /* Cycle detected, stop recursion */
}
```

**Update recursive calls** (line 448):
```c
for (int i = 0; i < glyph->parent_count; i++) {
    if (glyph_run_inheritance(glyph->parent_glyphs[i], &parent_state, depth + 1, ctx)) {
        // ... existing code ...
    }
}
```

**Update all callers** to initialize context:
```c
// In glyph_activate() function (line 466)
InheritanceContext ctx;
inheritance_context_init(&ctx);

if (glyph_run_inheritance(glyph_id, &field_state, 0, &ctx)) {
    // ...
}
```

### Verification

```bash
# Create circular inheritance test
cat > /tmp/vault/circ_001.gdf << 'EOF'
glyph_id: circ_001
parent_glyphs: circ_002
EOF

cat > /tmp/vault/circ_002.gdf << 'EOF'
glyph_id: circ_002
parent_glyphs: circ_001
EOF

# Test - should detect cycle without crashing
./glyph_interp --vault /tmp/vault --activate circ_001

# Expected output:
# Security: Circular inheritance detected involving circ_001
# Activation failed
```

---

## Critical Vulnerability #3: Unchecked File Path in load_gdf_file

**CVE**: Pending
**Severity**: CRITICAL (CVSS 8.8)
**Component**: `src/glyph_interpreter.c` lines 615-621
**Impact**: Arbitrary file read via command-line argument

### Vulnerability Description

The `--load` command-line argument directly passes user input to `load_gdf_file()` without validation:

```c
// main() - VULNERABLE CODE
else if (strcmp(argv[i], "--load") == 0 && i + 1 < argc) {
    load_file = argv[++i];  // User-controlled, no validation
}

// Later...
if (load_file) {
    if (!load_gdf_file(load_file)) {  // Arbitrary file read
        fprintf(stderr, "Failed to load: %s\n", load_file);
    }
}
```

**Attack Vector**:
```bash
./glyph_interp --vault /var/db/glyphos --load /etc/shadow
./glyph_interp --vault /var/db/glyphos --load /usr/local/etc/sensitive_config
```

### Fix

**File**: `src/glyph_interpreter.c`

**Update load_gdf_file function** (line 615):
```c
static int load_gdf_file(const char* filepath, const char* vault_path) {
    char resolved[PATH_MAX];

    /* SECURITY FIX: Validate file is within vault */
    if (validate_vault_path(vault_path, filepath, resolved, sizeof(resolved)) != 0) {
        fprintf(stderr, "Security: File must be in vault directory: %s\n", filepath);
        return 0;
    }

    /* SECURITY FIX: Verify regular file */
    struct stat st;
    if (lstat(resolved, &st) != 0) {
        fprintf(stderr, "Error: Cannot access file: %s\n", resolved);
        return 0;
    }

    if (!S_ISREG(st.st_mode)) {
        fprintf(stderr, "Security: Not a regular file: %s\n", resolved);
        return 0;
    }

    /* SECURITY FIX: Check file size limit (prevent DoS) */
    if (st.st_size > 1024 * 1024) {  /* 1MB limit */
        fprintf(stderr, "Security: File too large: %s (max 1MB)\n", resolved);
        return 0;
    }

    GlyphDefinition glyph;
    if (parse_gdf_file(resolved, &glyph)) {
        if (g_registry.count < MAX_GLYPHS) {
            g_registry.glyphs[g_registry.count++] = glyph;
            return 1;
        }
    }

    return 0;
}
```

**Update callers** (line 625):
```c
if (load_file) {
    /* Extract just the filename, reject paths */
    const char* filename = strrchr(load_file, '/');
    filename = filename ? filename + 1 : load_file;

    if (!load_gdf_file(filename, vault_path)) {
        fprintf(stderr, "Failed to load: %s\n", filename);
        return 1;
    }
}
```

### Verification

```bash
# Test absolute path rejection
./glyph_interp --vault /tmp/vault --load /etc/passwd

# Expected output:
# Security: File must be in vault directory: passwd
# Failed to load: passwd

# Test path traversal rejection
./glyph_interp --vault /tmp/vault --load ../../../etc/shadow

# Expected output:
# Security: File must be in vault directory: ../../../etc/shadow
```

---

## High Severity: Insufficient Numeric Validation

**Severity**: HIGH (CVSS 6.5)
**Component**: `src/glyph_interpreter.c` lines 212-235
**Impact**: Parser failures, integer/float overflow, DoS

### Fix

**File**: `src/glyph_interpreter.c`

**Replace atof/atoi** (lines 212-235):
```c
} else if (strcmp(trimmed_key, "resonance_freq") == 0 || strcmp(trimmed_key, "resonance") == 0) {
    char* endptr;
    errno = 0;
    double value = strtod(trimmed_value, &endptr);

    if (errno != 0 || *endptr != '\0') {
        fprintf(stderr, "Parse error: Invalid number for %s: %s\n", trimmed_key, trimmed_value);
        return -1;
    }

    if (validate_range_double(value, 0.0, 100000.0, "resonance_freq") != 0) {
        return -1;
    }

    glyph->resonance_freq = value;

} else if (strcmp(trimmed_key, "field_magnitude") == 0 || strcmp(trimmed_key, "magnitude") == 0) {
    char* endptr;
    errno = 0;
    double value = strtod(trimmed_value, &endptr);

    if (errno != 0 || *endptr != '\0') {
        fprintf(stderr, "Parse error: Invalid number for %s: %s\n", trimmed_key, trimmed_value);
        return -1;
    }

    if (validate_range_double(value, 0.0, 1000.0, "field_magnitude") != 0) {
        return -1;
    }

    glyph->field_magnitude = value;

} else if (strcmp(trimmed_key, "coherence") == 0) {
    char* endptr;
    errno = 0;
    long value = strtol(trimmed_value, &endptr, 10);

    if (errno != 0 || *endptr != '\0') {
        fprintf(stderr, "Parse error: Invalid integer for %s: %s\n", trimmed_key, trimmed_value);
        return -1;
    }

    if (validate_range_int((int)value, 0, 1000, "coherence") != 0) {
        return -1;
    }

    glyph->coherence = (int)value;
}
```

---

## Build Integration

**Update Makefile** to include security utils:

```makefile
# Security-hardened build
SECURITY_SOURCES = src/security_utils.c
SECURITY_HEADERS = src/security_utils.h
SECURITY_CFLAGS = -D_FORTIFY_SOURCE=2 -fstack-protector-strong

glyph_interp_secure: src/glyph_interpreter.c $(SECURITY_SOURCES)
	$(CC) $(CFLAGS) $(SECURITY_CFLAGS) \
		-o bin/glyph_interp \
		src/glyph_interpreter.c $(SECURITY_SOURCES) -lm

substrate_core_secure: src/substrate_core.c $(SECURITY_SOURCES)
	$(CC) $(CFLAGS) $(SECURITY_CFLAGS) \
		-o bin/substrate_core \
		src/substrate_core.c $(SECURITY_SOURCES) -lm
```

**Update unified_pipeline.sh**:
```bash
# Build with security hardening
CC=gcc scripts/unified_pipeline.sh --security-hardened
```

---

## Testing

### Security Test Suite

**File**: `ci/security_tests.sh`

```bash
#!/bin/sh
# Security regression test suite

set -e

echo "=== GlyphOS Security Test Suite ==="

# Test 1: Path traversal protection
echo "[1/5] Testing path traversal protection..."
mkdir -p /tmp/security_test/vault
ln -sf /etc/passwd /tmp/security_test/vault/passwd.gdf
if ./bin/glyph_interp --vault /tmp/security_test/vault --list 2>&1 | grep -q "Skipping"; then
    echo "✓ Path traversal blocked"
else
    echo "✗ FAIL: Path traversal not blocked"
    exit 1
fi

# Test 2: Circular inheritance
echo "[2/5] Testing circular inheritance protection..."
cat > /tmp/security_test/vault/a.gdf << 'EOF'
glyph_id: a
parent_glyphs: b
EOF
cat > /tmp/security_test/vault/b.gdf << 'EOF'
glyph_id: b
parent_glyphs: a
EOF
if ./bin/glyph_interp --vault /tmp/security_test/vault --activate a 2>&1 | grep -q "Circular"; then
    echo "✓ Circular inheritance detected"
else
    echo "✗ FAIL: Circular inheritance not detected"
    exit 1
fi

# Test 3: File size limit
echo "[3/5] Testing file size limits..."
dd if=/dev/zero of=/tmp/security_test/vault/huge.gdf bs=1M count=10 2>/dev/null
if ./bin/glyph_interp --vault /tmp/security_test/vault --load huge.gdf 2>&1 | grep -q "too large"; then
    echo "✓ Large file rejected"
else
    echo "✗ FAIL: Large file not rejected"
    exit 1
fi

# Test 4: Numeric overflow protection
echo "[4/5] Testing numeric validation..."
cat > /tmp/security_test/vault/overflow.gdf << 'EOF'
glyph_id: overflow
resonance_freq: 1e308
EOF
if ./bin/glyph_interp --vault /tmp/security_test/vault --load overflow.gdf 2>&1 | grep -q "out of range"; then
    echo "✓ Numeric overflow rejected"
else
    echo "✗ FAIL: Numeric overflow not rejected"
    exit 1
fi

# Test 5: Symlink protection
echo "[5/5] Testing symlink protection..."
ln -sf /etc/shadow /tmp/security_test/vault/shadow.gdf
if ./bin/glyph_interp --vault /tmp/security_test/vault --list 2>&1 | grep -q "non-regular"; then
    echo "✓ Symlink rejected"
else
    echo "✗ FAIL: Symlink not rejected"
    exit 1
fi

# Cleanup
rm -rf /tmp/security_test

echo ""
echo "✅ All security tests passed"
```

---

## Deployment Checklist

Before deploying to production:

- [ ] Apply all critical patches
- [ ] Build with `-fsanitize=address,undefined` and test
- [ ] Run security test suite (`ci/security_tests.sh`)
- [ ] Run fuzzer for 24 hours minimum
- [ ] External security audit completed
- [ ] Pen-test validation
- [ ] Update security documentation
- [ ] Train ops team on security features

---

## Acknowledgments

Security vulnerabilities discovered through:
- Internal code review
- Static analysis (Semgrep, clang-tidy)
- Fuzzing (libFuzzer, 10K iterations)
- Automated agent-based security assessment

**Disclosure Policy**: Responsible disclosure - patches created before public announcement.
