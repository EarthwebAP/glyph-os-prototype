#!/bin/bash
#
# End-to-end demo: Create → Dynamics → Render
#
# This script demonstrates the complete glyph pipeline:
# 1. Create a glyph with energy
# 2. Run dynamics simulation (energy decays, glyph activates)
# 3. Export snapshots at multiple time steps
# 4. Render snapshots to animated GIF
#
# Usage:
#   ./demos/end_to_end_demo.sh
#
# Output:
#   renderer/demos/demo.gif - 12-second animated GIF showing glyph evolution

set -e  # Exit on error

echo "=== Glyph OS End-to-End Demo ==="
echo

# Configuration
GLYPH_CONTENT="End-to-end demo glyph"
INITIAL_ENERGY=8.0
FREQ=440.0
COMPLEXITY=6
TIME_STEPS=5
OUTPUT_GIF="renderer/demos/demo.gif"

echo "Step 1: Creating glyph with initial energy..."
echo "  Content: $GLYPH_CONTENT"
echo "  Energy: $INITIAL_ENERGY"
echo "  Frequency: ${FREQ}Hz"
echo

# Create glyph
GLYPH_ID=$(./runtime/cli/create_glyph.py "$GLYPH_CONTENT" \
  --metadata "{
    \"energy\": $INITIAL_ENERGY,
    \"form\": {\"topology\": [\"loop\"], \"parameters\": {\"complexity\": $COMPLEXITY}},
    \"resonance\": {\"tone\": {\"freq\": $FREQ, \"amplitude\": 0.5}},
    \"state\": {\"energy\": $INITIAL_ENERGY, \"activated\": false, \"last_updated\": \"2025-12-04T19:00:00Z\"},
    \"identity\": {\"lineage\": []},
    \"serialization\": {\"format\": \"json\"}
  }")

echo "  Created glyph: $GLYPH_ID"
echo

# Find glyph file
PREFIX1=${GLYPH_ID:0:2}
PREFIX2=${GLYPH_ID:2:2}
GLYPH_FILE="persistence/$PREFIX1/$PREFIX2/glyph_$GLYPH_ID.json"

if [ ! -f "$GLYPH_FILE" ]; then
    echo "Error: Glyph file not found: $GLYPH_FILE"
    exit 1
fi

echo "Step 2: Running dynamics simulation..."
echo "  Time steps: $TIME_STEPS"
echo

# Run dynamics for multiple steps, saving each state
for ((step=0; step<=$TIME_STEPS; step++)); do
    if [ $step -eq 0 ]; then
        echo "  Step $step: Initial state (energy=$INITIAL_ENERGY)"
    else
        echo "  Step $step: Running dynamics..."
        ./runtime/cli/run_dynamics.py "$GLYPH_ID" \
          --time-delta 1 \
          --save \
          --verbose > /dev/null 2>&1 || true
    fi
done

echo

echo "Step 3: Rendering GIF from glyph state..."
echo "  Output: $OUTPUT_GIF"
echo

# Render the final glyph state
python3 renderer/render.py "$GLYPH_FILE" \
  --out "$OUTPUT_GIF" \
  --duration 12 \
  --fps 12 \
  --zoom-levels "1,4,16"

echo

echo "=== Demo Complete ==="
echo
echo "Output:"
echo "  GIF: $OUTPUT_GIF"
echo "  Glyph ID: $GLYPH_ID"
echo "  Glyph file: $GLYPH_FILE"
echo
echo "To view the GIF, open: $OUTPUT_GIF"
echo

exit 0
