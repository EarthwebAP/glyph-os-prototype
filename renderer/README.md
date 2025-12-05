# Glyph Renderer

Parametric renderer for glyphs. Reads persisted glyphs and generates visual representations based on glyph properties.

## Rendering Rules

The renderer uses the following mappings from glyph fields to visual properties:

| Glyph Field | Visual Property | Mapping |
|-------------|-----------------|---------|
| `state.energy` | Size | Linear: `base_size + (energy × scale)` |
| `state.energy` | Brightness | Linear: `0.3 + (normalized_energy × 0.7)` |
| `resonance.tone.freq` | Color Hue | Logarithmic: 20Hz→red, 440Hz→cyan, 20kHz→red |
| `form.topology[0]` | Base Shape | node→circle, edge→line, loop→ring, etc. |
| `form.parameters.complexity` | Polygon Sides | Used for mesh/polygon shapes |
| `state.activated` | Pulse Effect | Animated rings when activated |

### Shape Mappings

- **node** → Filled circle with glow
- **edge** → Cross/line pattern
- **loop** → Concentric rings
- **mesh** → Regular polygon (n-sided)
- **surface** → Filled polygon
- **void** → Outline only

## Installation

```bash
pip install Pillow
```

## Usage

### Basic Rendering

Render a single glyph to GIF:

```bash
python3 renderer/render.py <glyph_path> --out output.gif
```

### Full Options

```bash
python3 renderer/render.py <glyph_path> \
  --out output.gif \
  --duration 12 \
  --fps 12 \
  --width 800 \
  --height 800 \
  --zoom-levels 1,4,16
```

### Rendering from NVMe Persistence

To render a persisted glyph by ID:

```bash
# Get the glyph ID (e.g., from create_glyph.py output)
GLYPH_ID="a19315151ebbf7a9b020b9dc6e52b511f168afe7cb11f53b7c2c101bf481fedd"

# Find the glyph file (with Merkle directory structure)
GLYPH_PATH="/mnt/persistence/a1/93/glyph_${GLYPH_ID}.json"
# Or fallback to local persistence:
GLYPH_PATH="persistence/a1/93/glyph_${GLYPH_ID}.json"

# Render to GIF
python3 renderer/render.py "$GLYPH_PATH" \
  --out renderer/demos/my_glyph.gif \
  --duration 12 \
  --fps 12
```

## Demo GIF Reproduction

To reproduce the demo GIF from the example glyph:

```bash
# Using the example glyph
python3 renderer/render.py spec/examples/glyph_example.json \
  --out renderer/demos/demo.gif \
  --duration 12 \
  --fps 12

# Or from a persisted glyph ID
python3 renderer/render.py persistence/a1/93/glyph_a19315151ebbf7a9b020b9dc6e52b511f168afe7cb11f53b7c2c101bf481fedd.json \
  --out renderer/demos/demo.gif \
  --duration 12 \
  --fps 12
```

This creates a 12-second GIF at 12 fps showing the glyph at 3 zoom levels (1x, 4x, 16x).

## Parameters

| Parameter | Default | Description |
|-----------|---------|-------------|
| `glyph_path` | (required) | Path to glyph JSON file |
| `--out` | output.gif | Output GIF file path |
| `--duration` | 12 | Total duration in seconds |
| `--fps` | 12 | Frames per second |
| `--width` | 800 | Image width in pixels |
| `--height` | 800 | Image height in pixels |
| `--zoom-levels` | 1,4,16 | Comma-separated zoom levels |

## Output

The renderer creates an animated GIF showing:

1. **Multi-scale visualization**: Frames at different zoom levels (default: 1x, 4x, 16x)
2. **Scale independence**: Demonstrates that glyphs render correctly at all scales
3. **Parameter-driven visuals**: Size, color, and shape all derived from glyph properties
4. **Activation indicators**: Pulsing rings when `state.activated = true`

### Frame Distribution

For a 12-second GIF at 12 fps with 3 zoom levels:
- Total frames: 144
- Frames per zoom level: 48
- Order: [1x × 48] → [4x × 48] → [16x × 48]

## Examples

### Low Energy Glyph (energy=1.0)
- Small size
- Dim brightness
- Based on frequency: 440Hz → cyan hue

### High Energy Glyph (energy=10.0)
- Large size
- Bright, vibrant
- If activated: pulsing rings

### Different Topologies
- **node**: `{"topology": ["node"]}` → Circle
- **edge**: `{"topology": ["edge"]}` → Cross/line
- **loop**: `{"topology": ["loop"]}` → Concentric rings

## Integration with Dynamics

The renderer visualizes dynamic glyph state:

```bash
# Create a glyph with energy
./runtime/cli/create_glyph.py "Dynamic glyph" \
  --metadata '{"energy": 5.0, "resonance": {"tone": {"freq": 880}}}'

# Output: glyph ID

# Run dynamics (energy decays)
./runtime/cli/run_dynamics.py <glyph_id> --time-delta 5 --save

# Render evolved glyph
python3 renderer/render.py persistence/xx/yy/glyph_<id>.json \
  --out evolved.gif
```

## Workflow: Create → Dynamics → Render

Complete workflow demonstrating the full pipeline:

```bash
# 1. Create glyph with initial energy
GLYPH_ID=$(./runtime/cli/create_glyph.py "Evolving glyph" \
  --metadata '{
    "energy": 8.0,
    "form": {"topology": ["loop"], "parameters": {"complexity": 6}},
    "resonance": {"tone": {"freq": 440.0}}
  }')

# 2. Run dynamics simulation
./runtime/cli/run_dynamics.py "$GLYPH_ID" \
  --time-delta 10 \
  --save \
  --verbose

# 3. Render to GIF
GLYPH_PATH="persistence/${GLYPH_ID:0:2}/${GLYPH_ID:2:2}/glyph_${GLYPH_ID}.json"
python3 renderer/render.py "$GLYPH_PATH" \
  --out result.gif \
  --duration 12 \
  --fps 12
```

## Technical Details

### Color Mapping
- Frequency-to-hue uses logarithmic scale for perceptual uniformity
- HSV color space: H from frequency, S=0.8 (vibrant), V from energy
- RGB conversion via `colorsys` standard library

### Shape Rendering
- PIL/Pillow ImageDraw for vector-like shapes
- Glow effects via multiple outline rings with decreasing alpha
- Anti-aliasing via Pillow's default settings

### Performance
- 800×800 frame: ~5-10ms render time
- 144 frames (12s @ 12fps): ~1-2 seconds total
- GIF compression: ~24KB for demo (144 frames)

## Future Enhancements

Potential additions (not yet implemented):
- Particle effects based on `state.energy`
- Animation interpolation between dynamics steps
- 3D rendering for complex topologies
- WebGL/SVG export formats
- Real-time preview mode
