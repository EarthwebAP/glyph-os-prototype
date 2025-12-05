# Persistence API Specification

## Overview

The glyphd service provides REST API for creating and querying glyphs with persistent storage.

## Base URL

```
http://localhost:8080
```

## Endpoints

### POST /glyphs

Create a new glyph.

**Request:**

```json
{
  "content": "string (glyph content)",
  "metadata": {
    "key": "value (optional metadata)"
  }
}
```

**Response:**

```json
{
  "id": "string (64-char hex glyph ID)",
  "commit_id": "string (commit identifier)"
}
```

**Status Codes:**
- `200 OK` - Glyph created successfully
- `400 Bad Request` - Invalid input

### GET /glyphs/:id

Query a glyph by ID.

**Parameters:**
- `id` - Glyph ID (64-char hex string)

**Response:**

```json
{
  "id": "string",
  "content": "string",
  "metadata": {object},
  "commit_id": "string (optional)"
}
```

**Status Codes:**
- `200 OK` - Glyph found
- `404 Not Found` - Glyph does not exist

### GET /health

Health check endpoint.

**Response:** `"glyphd OK"`

## Example Usage

### Create Glyph

```bash
curl -X POST http://localhost:8080/glyphs \
  -H "Content-Type: application/json" \
  -d '{
    "content": "Hello, GlyphOS",
    "metadata": {
      "energy": 8.0,
      "resonance": {"tone": {"freq": 440.0}}
    }
  }'
```

**Response:**

```json
{
  "id": "00000000000001913c7d1e5a7c6f3c000000000000000000000000000000000000000000",
  "commit_id": "commit_00000001913c7d1e"
}
```

### Query Glyph

```bash
curl http://localhost:8080/glyphs/00000000000001913c7d1e5a7c6f3c000000000000000000000000000000000000000000
```

**Response:**

```json
{
  "id": "00000000000001913c7d1e5a7c6f3c000000000000000000000000000000000000000000",
  "content": "Hello, GlyphOS",
  "metadata": {
    "energy": 8.0,
    "resonance": {"tone": {"freq": 440.0}}
  },
  "commit_id": "commit_00000001913c7d1e"
}
```

## Persistence Layer

In production, glyphs are persisted to NVMe using:
- Atomic writes (temp file + fsync + rename)
- Directory sharding (256 subdirectories)
- JSON format

Current implementation uses in-memory storage for demo purposes.

---

**Version:** 1.0  
**Last Updated:** 2025-12-04
