# Glyph OS Prototype

A prototype implementation of Glyph OS with content-addressed glyph storage.

## Sprint Scope: Phase 0 Runtime Stub

**Objective:** Deliver a minimal runnable CLI that creates and persists glyphs using content-addressing.

**Deliverables:**
- ✅ `runtime/cli/create_glyph.py` - Creates glyphs with SHA256 content addressing
- ✅ `runtime/cli/query_glyph.py` - Queries glyphs by ID
- ✅ `runtime/tests/test_create_query.py` - Unit tests for create and query
- ✅ `spec/glyph_spec_v0.yaml` - Glyph specification v0
- ✅ CI workflow via GitHub Actions

**Acceptance Criteria:**
- `./runtime/cli/create_glyph.py <content>` prints glyph ID
- Glyph persisted as `persistence/glyph_<id>.json`
- `./runtime/cli/query_glyph.py <id>` retrieves glyph by ID
- Tests pass locally and in CI
- Tagged as `phase0-runtime-stub`

## Structure

```
glyph-os-prototype/
├── runtime/
│   ├── cli/
│   │   ├── create_glyph.py     # Create glyphs with SHA256 addressing
│   │   ├── query_glyph.py      # Query glyphs by ID
│   │   └── run_dynamics.py     # Run dynamics engine on glyphs
│   ├── dynamics/
│   │   └── engine.py           # Deterministic dynamics engine
│   └── tests/
│       ├── test_create_query.py # Persistence tests
│       └── test_dynamics.py     # Dynamics property tests
├── persistence/                 # Storage (Merkle-organized)
├── spec/
│   └── glyph_spec_v0.yaml      # Glyph specification
└── .github/
    └── workflows/
        └── test.yml            # CI configuration
```

## Usage

### Create a Glyph

```bash
python3 runtime/cli/create_glyph.py "Hello, World!"
```

With metadata:

```bash
python3 runtime/cli/create_glyph.py "Hello, World!" --metadata '{"author": "user"}'
```

### Query a Glyph

```bash
python3 runtime/cli/query_glyph.py <glyph-id>
```

### Run Dynamics Engine

Apply dynamics rules to a persisted glyph:

```bash
python3 runtime/cli/run_dynamics.py <glyph-id> --time-delta 5 --verbose
```

## Features

### NVMe Persistence with Atomic Writes

Glyphs are persisted with:
- **NVMe priority**: Checks `/mnt/persistence` first, falls back to `./persistence`
- **Atomic writes**: Uses temp files + rename to prevent corruption
- **Merkle organization**: Files stored in `<prefix1>/<prefix2>/glyph_<id>.json` for performance

Example: ID `a1b2c3...` → `persistence/a1/b2/glyph_a1b2c3....json`

### Dynamics Engine

Deterministic rule engine with three core rules:

1. **Activation Threshold**: Glyphs activate when energy ≥ threshold
2. **Merge Precedence**: Higher energy glyphs dominate in merges
3. **Decay**: Energy decays exponentially over time: `E_new = E_old * (1 - rate)^Δt`

All rules are:
- ✅ Deterministic (same input → same output)
- ✅ Property-tested (13 property tests)
- ✅ Composable (rules combine predictably)

## Testing

Run all tests:

```bash
python3 -m unittest discover runtime/tests -v
```

Test suite:
- ✅ 2 persistence tests (create/query)
- ✅ 13 dynamics property tests
- ✅ Total: 15 tests, all passing

## Content Addressing

Glyphs are stored using SHA256 content addressing. The glyph ID is the SHA256 hash of the content, ensuring:
- Deterministic IDs: same content always produces same ID
- Content integrity: ID verifies content hasn't changed
- Deduplication: identical content creates only one glyph
