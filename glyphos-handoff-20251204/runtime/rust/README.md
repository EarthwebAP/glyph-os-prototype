# Glyph OS Rust Runtime Components

This directory contains the Rust-based runtime components for Glyph OS.

## Components

### 1. glyphd - Main Daemon Service

REST API service for creating and querying glyphs.

**Endpoints:**
- `POST /glyphs` - Create a new glyph
  - Request: `{"content": "string", "energy": 1.0}`
  - Response: `{"id": "glyph_xxx", "commit_id": "commit_xxx", "status": "created"}`
- `GET /glyphs/:id` - Get a glyph by ID
  - Response: Full glyph object with all metadata
- `GET /health` - Health check
  - Response: `{"status": "healthy", "service": "glyphd", "version": "0.1.0"}`

**Port:** 8080

### 2. glyph-spu - SPU Service (Symbolic Processing Unit)

Hardware-accelerated merge operations service.

**Endpoints:**
- `POST /offload/merge` - Merge two glyphs
  - Request: `{"glyph1": {...}, "glyph2": {...}}`
  - Response: `{"merged_state": {...}, "operation": "merge", "latency_ns": 1234}`
- `GET /health` - Health check

**Port:** 8081

**Merge Algorithm:**
- Higher energy glyph content comes first
- Energy values are summed (energy conservation)
- Activation count is max of both glyphs
- Last update time is max of both glyphs
- Content concatenated with " + " separator
- Parent IDs tracked (parent1 = higher energy)

## Building

### Prerequisites

Install Rust toolchain:
```bash
curl --proto '=https' --tlsv1.2 -sSf https://sh.rustup.rs | sh
source $HOME/.cargo/env
```

### Build Both Services

```bash
# Build glyphd
cd glyphd
cargo build --release

# Build glyph-spu
cd ../glyph-spu
cargo build --release
```

### Run Tests

```bash
# Test glyphd
cd glyphd
cargo test

# Test glyph-spu
cd ../glyph-spu
cargo test
```

## Running

### Start glyphd

```bash
cd glyphd
cargo run --release
# Listens on http://0.0.0.0:8080
```

### Start glyph-spu

```bash
cd glyph-spu
cargo run --release
# Listens on http://0.0.0.0:8081
```

## Example Usage

### Create a Glyph

```bash
curl -X POST http://localhost:8080/glyphs \
  -H "Content-Type: application/json" \
  -d '{"content": "Hello Glyph OS", "energy": 2.5}'
```

Response:
```json
{
  "id": "glyph_0000000000000001",
  "commit_id": "commit_0123456789abcdef",
  "status": "created"
}
```

### Query a Glyph

```bash
curl http://localhost:8080/glyphs/glyph_0000000000000001
```

Response:
```json
{
  "id": "glyph_0000000000000001",
  "content": "Hello Glyph OS",
  "energy": 2.5,
  "activation_count": 0,
  "last_update_time": 1701734400,
  "commit_id": "commit_0123456789abcdef"
}
```

### Merge Two Glyphs

```bash
curl -X POST http://localhost:8081/offload/merge \
  -H "Content-Type: application/json" \
  -d '{
    "glyph1": {
      "id": "id1",
      "content": "alpha",
      "content_len": 5,
      "energy": 2.0,
      "activation_count": 5,
      "last_update_time": 100
    },
    "glyph2": {
      "id": "id2",
      "content": "beta",
      "content_len": 4,
      "energy": 3.0,
      "activation_count": 3,
      "last_update_time": 200
    }
  }'
```

Response:
```json
{
  "merged_state": {
    "content": "beta + alpha",
    "content_len": 12,
    "energy": 5.0,
    "activation_count": 5,
    "last_update_time": 200,
    "parent1_id": "id2",
    "parent2_id": "id1"
  },
  "operation": "merge",
  "latency_ns": 12345
}
```

## Implementation Details

### glyphd

- **Framework:** Axum web framework with Tokio async runtime
- **Storage:** In-memory HashMap protected by RwLock
- **ID Generation:** Sequential hex-encoded IDs
- **Commit IDs:** Timestamp-based unique identifiers
- **Logging:** Structured logging with tracing

### glyph-spu

- **Framework:** Axum web framework with Tokio async runtime
- **Algorithm:** Simulates FPGA merge accelerator logic from `fpga/hls/merge_accelerator.cpp`
- **Performance:** Reports latency in nanoseconds
- **Test Coverage:** Matches test vectors from `fpga/sim/test_vectors/merge_inputs.json`

### Merge Logic Details

The merge algorithm follows the hardware specification:

1. **Energy-based ordering:** The glyph with higher energy appears first in merged content
2. **Energy conservation:** Total energy = energy1 + energy2
3. **Metadata aggregation:**
   - `activation_count` = max(count1, count2)
   - `last_update_time` = max(time1, time2)
4. **Content concatenation:** `{higher_energy_content} + {lower_energy_content}`
5. **Parent tracking:** parent1_id = higher energy glyph, parent2_id = lower energy glyph

## Dependencies

- `tokio`: Async runtime
- `axum`: Web framework
- `serde`/`serde_json`: Serialization
- `tracing`: Logging
- `tower`: Middleware
- `anyhow`: Error handling

## Architecture

```
┌─────────────┐         ┌──────────────┐
│   Client    │────────▶│   glyphd     │
└─────────────┘         │   :8080      │
                        └──────────────┘
                              │
                              │ (offload merge)
                              ▼
                        ┌──────────────┐
                        │  glyph-spu   │
                        │   :8081      │
                        └──────────────┘
                              │
                              ▼
                        ┌──────────────┐
                        │ FPGA/HLS     │
                        │ (simulated)  │
                        └──────────────┘
```

## Testing

Both services include comprehensive unit tests:

- **glyphd tests:** Glyph creation, default values
- **glyph-spu tests:**
  - Higher energy ordering
  - Energy conservation
  - Equal energy handling
  - Metadata aggregation

Run all tests:
```bash
cargo test --all
```

## Production Deployment

For production use:

1. Replace in-memory storage with persistent database (PostgreSQL/Redis)
2. Add authentication/authorization
3. Implement actual FPGA communication in glyph-spu
4. Add monitoring and metrics (Prometheus)
5. Configure TLS/HTTPS
6. Add rate limiting
7. Implement proper error handling and retries
