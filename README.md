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
│   └── cli/
│       ├── create_glyph.py  # Create glyphs with SHA256 addressing
│       └── query_glyph.py   # Query glyphs by ID
├── persistence/             # Storage for glyph JSON files
├── tests/
│   └── test_glyph_cli.py   # Unit tests
└── .github/
    └── workflows/
        └── test.yml         # CI configuration
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

## Testing

Run unit tests:

```bash
python3 -m unittest tests.test_glyph_cli -v
```

## Content Addressing

Glyphs are stored using SHA256 content addressing. The glyph ID is the SHA256 hash of the content, ensuring:
- Deterministic IDs: same content always produces same ID
- Content integrity: ID verifies content hasn't changed
- Deduplication: identical content creates only one glyph
