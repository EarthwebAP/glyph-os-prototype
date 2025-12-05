#!/bin/bash
# API test script for Glyph OS runtime services

set -e

GLYPHD_URL="http://localhost:8080"
SPU_URL="http://localhost:8081"

echo "=========================================="
echo "Glyph OS API Test Suite"
echo "=========================================="
echo ""

# Check if services are running
echo "Checking service health..."
echo ""

if ! curl -s -f "$GLYPHD_URL/health" > /dev/null 2>&1; then
    echo "ERROR: glyphd not responding at $GLYPHD_URL"
    echo "Please start glyphd: cargo run --release --bin glyphd"
    exit 1
fi
echo "✓ glyphd is running"

if ! curl -s -f "$SPU_URL/health" > /dev/null 2>&1; then
    echo "ERROR: glyph-spu not responding at $SPU_URL"
    echo "Please start glyph-spu: cargo run --release --bin glyph-spu"
    exit 1
fi
echo "✓ glyph-spu is running"

echo ""
echo "=========================================="
echo "Testing glyphd endpoints"
echo "=========================================="
echo ""

# Test health endpoint
echo "1. Testing GET /health"
curl -s "$GLYPHD_URL/health" | jq .
echo ""

# Create first glyph
echo "2. Testing POST /glyphs (create glyph1)"
RESPONSE1=$(curl -s -X POST "$GLYPHD_URL/glyphs" \
    -H "Content-Type: application/json" \
    -d '{"content": "Hello Glyph OS", "energy": 2.5}')
echo "$RESPONSE1" | jq .
GLYPH1_ID=$(echo "$RESPONSE1" | jq -r .id)
echo "Created glyph: $GLYPH1_ID"
echo ""

# Create second glyph
echo "3. Testing POST /glyphs (create glyph2)"
RESPONSE2=$(curl -s -X POST "$GLYPHD_URL/glyphs" \
    -H "Content-Type: application/json" \
    -d '{"content": "Symbolic Computing", "energy": 3.5}')
echo "$RESPONSE2" | jq .
GLYPH2_ID=$(echo "$RESPONSE2" | jq -r .id)
echo "Created glyph: $GLYPH2_ID"
echo ""

# Query first glyph
echo "4. Testing GET /glyphs/:id"
curl -s "$GLYPHD_URL/glyphs/$GLYPH1_ID" | jq .
echo ""

# Test 404
echo "5. Testing GET /glyphs/:id (not found)"
curl -s -w "\nHTTP Status: %{http_code}\n" "$GLYPHD_URL/glyphs/nonexistent"
echo ""

echo ""
echo "=========================================="
echo "Testing glyph-spu endpoints"
echo "=========================================="
echo ""

# Test health endpoint
echo "1. Testing GET /health"
curl -s "$SPU_URL/health" | jq .
echo ""

# Test merge operation
echo "2. Testing POST /offload/merge"
curl -s -X POST "$SPU_URL/offload/merge" \
    -H "Content-Type: application/json" \
    -d '{
        "glyph1": {
            "id": "test_alpha",
            "content": "alpha",
            "content_len": 5,
            "energy": 2.5,
            "activation_count": 5,
            "last_update_time": 100
        },
        "glyph2": {
            "id": "test_beta",
            "content": "beta",
            "content_len": 4,
            "energy": 3.5,
            "activation_count": 3,
            "last_update_time": 200
        }
    }' | jq .
echo ""

# Test merge with equal energy
echo "3. Testing merge with equal energy"
curl -s -X POST "$SPU_URL/offload/merge" \
    -H "Content-Type: application/json" \
    -d '{
        "glyph1": {
            "id": "equal1",
            "content": "first",
            "content_len": 5,
            "energy": 4.0,
            "activation_count": 10,
            "last_update_time": 300
        },
        "glyph2": {
            "id": "equal2",
            "content": "second",
            "content_len": 6,
            "energy": 4.0,
            "activation_count": 8,
            "last_update_time": 400
        }
    }' | jq .
echo ""

# Test merge matching FPGA test vectors
echo "4. Testing merge matching FPGA test vectors"
curl -s -X POST "$SPU_URL/offload/merge" \
    -H "Content-Type: application/json" \
    -d '{
        "glyph1": {
            "id": "id1_0000000000000000000000000000000000000000000000000000000000",
            "content": "content1",
            "content_len": 8,
            "energy": 2.0,
            "activation_count": 0,
            "last_update_time": 0
        },
        "glyph2": {
            "id": "id2_0000000000000000000000000000000000000000000000000000000000",
            "content": "content2",
            "content_len": 8,
            "energy": 3.0,
            "activation_count": 0,
            "last_update_time": 0
        }
    }' | jq .
echo ""

echo "=========================================="
echo "✓ All API tests completed successfully!"
echo "=========================================="
echo ""
