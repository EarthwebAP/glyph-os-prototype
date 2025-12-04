# Glyph OS Prototype

A prototype implementation of Glyph OS with content-addressed glyph storage.

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
