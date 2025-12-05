# Glyph OS Rust Runtime - Quick Start Guide

## Prerequisites

### Install Rust
```bash
curl --proto '=https' --tlsv1.2 -sSf https://sh.rustup.rs | sh
source $HOME/.cargo/env
```

Verify installation:
```bash
rustc --version
cargo --version
```

## Quick Build & Run

### Option 1: Using the validation script
```bash
cd /home/daveswo/glyph-os-prototype/glyphos-handoff-20251204/runtime/rust
./validate.sh
```

This will:
- Check all files are present
- Build both services
- Run all tests
- Build release binaries

### Option 2: Manual build
```bash
cd /home/daveswo/glyph-os-prototype/glyphos-handoff-20251204/runtime/rust

# Build everything
cargo build --workspace --release

# Or build individually
cd glyphd && cargo build --release
cd ../glyph-spu && cargo build --release
```

## Running the Services

### Terminal 1: Start glyphd
```bash
cd /home/daveswo/glyph-os-prototype/glyphos-handoff-20251204/runtime/rust
cargo run --release --bin glyphd
```

Expected output:
```
2024-12-04T12:00:00.000000Z  INFO glyphd: Starting glyphd on 0.0.0.0:8080
2024-12-04T12:00:00.000000Z  INFO glyphd: Glyphd listening on 0.0.0.0:8080
```

### Terminal 2: Start glyph-spu
```bash
cd /home/daveswo/glyph-os-prototype/glyphos-handoff-20251204/runtime/rust
cargo run --release --bin glyph-spu
```

Expected output:
```
2024-12-04T12:00:00.000000Z  INFO glyph_spu: Starting glyph-spu on 0.0.0.0:8081
2024-12-04T12:00:00.000000Z  INFO glyph_spu: Glyph-SPU listening on 0.0.0.0:8081
```

### Terminal 3: Run API tests
```bash
cd /home/daveswo/glyph-os-prototype/glyphos-handoff-20251204/runtime/rust
./test-api.sh
```

## Manual API Testing

### Test glyphd

#### Health check
```bash
curl http://localhost:8080/health | jq .
```

Expected:
```json
{
  "status": "healthy",
  "service": "glyphd",
  "version": "0.1.0"
}
```

#### Create a glyph
```bash
curl -X POST http://localhost:8080/glyphs \
  -H "Content-Type: application/json" \
  -d '{"content": "Hello Glyph OS", "energy": 2.5}' | jq .
```

Expected:
```json
{
  "id": "glyph_0000000000000001",
  "commit_id": "commit_...",
  "status": "created"
}
```

#### Query a glyph (replace ID with actual ID from creation)
```bash
curl http://localhost:8080/glyphs/glyph_0000000000000001 | jq .
```

Expected:
```json
{
  "id": "glyph_0000000000000001",
  "content": "Hello Glyph OS",
  "energy": 2.5,
  "activation_count": 0,
  "last_update_time": 1701734400,
  "commit_id": "commit_..."
}
```

### Test glyph-spu

#### Health check
```bash
curl http://localhost:8081/health | jq .
```

Expected:
```json
{
  "status": "healthy",
  "service": "glyph-spu",
  "version": "0.1.0",
  "accelerator": "simulated"
}
```

#### Merge two glyphs
```bash
curl -X POST http://localhost:8081/offload/merge \
  -H "Content-Type: application/json" \
  -d '{
    "glyph1": {
      "id": "alpha",
      "content": "first",
      "content_len": 5,
      "energy": 2.0,
      "activation_count": 5,
      "last_update_time": 100
    },
    "glyph2": {
      "id": "beta",
      "content": "second",
      "content_len": 6,
      "energy": 3.0,
      "activation_count": 3,
      "last_update_time": 200
    }
  }' | jq .
```

Expected:
```json
{
  "merged_state": {
    "content": "second + first",
    "content_len": 13,
    "energy": 5.0,
    "activation_count": 5,
    "last_update_time": 200,
    "parent1_id": "beta",
    "parent2_id": "alpha"
  },
  "operation": "merge",
  "latency_ns": 12345
}
```

## Running Tests

### Run all tests
```bash
cargo test --workspace
```

### Run specific service tests
```bash
# Test glyphd
cd glyphd && cargo test

# Test glyph-spu
cd glyph-spu && cargo test
```

### Run tests with output
```bash
cargo test --workspace -- --nocapture
```

## Docker Deployment

### Build and run with Docker Compose
```bash
cd /home/daveswo/glyph-os-prototype/glyphos-handoff-20251204/runtime/rust
docker-compose up --build
```

### Run in background
```bash
docker-compose up -d
```

### Check logs
```bash
docker-compose logs -f
```

### Stop services
```bash
docker-compose down
```

## Performance Testing

### Simple benchmark with curl
```bash
# Benchmark glyph creation (100 requests)
for i in {1..100}; do
  curl -s -X POST http://localhost:8080/glyphs \
    -H "Content-Type: application/json" \
    -d "{\"content\": \"test $i\", \"energy\": $i}" \
    -o /dev/null -w "%{time_total}\n"
done | awk '{sum+=$1; count++} END {print "Avg time:", sum/count, "s"}'
```

### Benchmark merge operations
```bash
# Benchmark merge (100 requests)
for i in {1..100}; do
  curl -s -X POST http://localhost:8081/offload/merge \
    -H "Content-Type: application/json" \
    -d '{
      "glyph1": {"id": "a", "content": "alpha", "content_len": 5, "energy": 2.0, "activation_count": 0, "last_update_time": 0},
      "glyph2": {"id": "b", "content": "beta", "content_len": 4, "energy": 3.0, "activation_count": 0, "last_update_time": 0}
    }' \
    -o /dev/null -w "%{time_total}\n"
done | awk '{sum+=$1; count++} END {print "Avg time:", sum/count, "s"}'
```

## Troubleshooting

### Services won't start

#### Check if ports are available
```bash
# Check if port 8080 is in use
lsof -i :8080

# Check if port 8081 is in use
lsof -i :8081
```

#### Kill existing processes
```bash
# Kill process on port 8080
kill -9 $(lsof -ti:8080)

# Kill process on port 8081
kill -9 $(lsof -ti:8081)
```

### Build fails

#### Clean and rebuild
```bash
cargo clean
cargo build --workspace --release
```

#### Update dependencies
```bash
cargo update
```

### Tests fail

#### Run with verbose output
```bash
cargo test --workspace -- --nocapture
```

#### Check Rust version
```bash
rustc --version
# Should be 1.70.0 or higher
```

## Project Structure

```
runtime/rust/
├── Cargo.toml                  # Workspace configuration
├── README.md                   # Full documentation
├── QUICKSTART.md              # This file
├── validate.sh                # Validation script
├── test-api.sh                # API test script
├── docker-compose.yml         # Docker deployment
├── glyphd/
│   ├── Cargo.toml            # glyphd dependencies
│   ├── Dockerfile            # glyphd Docker image
│   └── src/
│       └── main.rs           # glyphd implementation
└── glyph-spu/
    ├── Cargo.toml            # glyph-spu dependencies
    ├── Dockerfile            # glyph-spu Docker image
    └── src/
        └── main.rs           # glyph-spu implementation
```

## Next Steps

1. Explore the full API documentation in `README.md`
2. Review the source code in `glyphd/src/main.rs` and `glyph-spu/src/main.rs`
3. Check out the FPGA test vectors in `../../fpga/sim/test_vectors/merge_inputs.json`
4. Build your own clients using the REST APIs
5. Integrate with the FPGA hardware accelerator

## Support

For issues or questions:
- Check `README.md` for detailed documentation
- Review test cases in the source code
- Check FPGA documentation in `../../fpga/`
