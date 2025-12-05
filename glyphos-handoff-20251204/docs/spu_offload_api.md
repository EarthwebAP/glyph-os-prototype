# SPU Offload API Specification

## Overview

The SPU offload service (`glyph-spu`) provides hardware-accelerated merge operations for glyphs. In production, this service interfaces with FPGA accelerators via DMA. For development/testing, it provides a software reference implementation.

## Base URL

```
http://localhost:8081
```

## Endpoints

### POST /offload/merge

Merge two glyphs using SPU acceleration.

**Request:**

```json
{
  "glyph1": {
    "id": "string (64 chars hex)",
    "content": "string (max 256 bytes)",
    "energy": float,
    "activation_count": uint32,
    "last_update_time": uint64
  },
  "glyph2": {
    "id": "string (64 chars hex)",
    "content": "string (max 256 bytes)",
    "energy": float,
    "activation_count": uint32,
    "last_update_time": uint64
  }
}
```

**Response:**

```json
{
  "merged_state": {
    "id": "string (new ID from hash)",
    "content": "string (concatenated)",
    "energy": float (sum),
    "activation_count": uint32 (max),
    "last_update_time": uint64 (max)
  },
  "parent1_id": "string",
  "parent2_id": "string"
}
```

**Status Codes:**
- `200 OK` - Merge successful
- `400 Bad Request` - Invalid input
- `500 Internal Server Error` - Processing failure

### GET /offload/status

Get SPU service status.

**Response:**

```json
{
  "service": "glyph-spu",
  "status": "ready",
  "accelerator": "software_reference" | "fpga_hw",
  "version": "0.1.0"
}
```

### GET /health

Health check endpoint.

**Response:** `"glyph-spu OK"`

## Example Usage

### curl

```bash
# Merge two glyphs
curl -X POST http://localhost:8081/offload/merge \
  -H "Content-Type: application/json" \
  -d '{
    "glyph1": {
      "id": "id1_000...",
      "content": "content1",
      "energy": 2.0,
      "activation_count": 0,
      "last_update_time": 0
    },
    "glyph2": {
      "id": "id2_000...",
      "content": "content2",
      "energy": 3.0,
      "activation_count": 0,
      "last_update_time": 0
    }
  }'

# Check status
curl http://localhost:8081/offload/status
```

### Python

```python
import requests

# Merge glyphs
response = requests.post('http://localhost:8081/offload/merge', json={
    'glyph1': {
        'id': 'id1_000...',
        'content': 'content1',
        'energy': 2.0,
        'activation_count': 0,
        'last_update_time': 0
    },
    'glyph2': {
        'id': 'id2_000...',
        'content': 'content2',
        'energy': 3.0,
        'activation_count': 0,
        'last_update_time': 0
    }
})

merged = response.json()
print(f"Merged ID: {merged['merged_state']['id']}")
print(f"Energy: {merged['merged_state']['energy']}")
```

## FPGA Integration

When `accelerator` is `"fpga_hw"`, the service uses DMA to offload merges to FPGA:

1. Service allocates DMA buffers for input/output
2. Writes glyph pair descriptors to input buffer
3. Writes buffer addresses to FPGA control registers
4. Sets AP_START=1 to begin processing
5. Polls AP_DONE or waits for interrupt
6. Reads merged result from output buffer
7. Returns result to caller

See `fpga/docs/dma_contract.json` for detailed DMA interface specification.

## Performance

| Configuration | Latency | Throughput |
|---------------|---------|------------|
| Software reference | ~5 µs | 200K ops/sec |
| FPGA (1 lane) | 355 ns | 200K ops/sec |
| FPGA (16 lanes) | 355 ns | 3.2M ops/sec |

## Error Handling

- Invalid glyph data → 400 Bad Request
- FPGA timeout → 500 Internal Server Error (falls back to software)
- DMA error → 500 Internal Server Error (falls back to software)

Software reference always available as fallback.

---

**Version:** 1.0  
**Last Updated:** 2025-12-04
