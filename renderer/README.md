# Glyph Renderer

Parametric renderer for visualizing glyph state as animated GIFs.

## Overvew

The renderer reads persisted glyph JSON files and creates visual representations based on:
- **Energy** → Size and brightness (linear mapping)
- **Resonance frequency** → Color hue (logarithmic mapping)
- **Topology** → Base shape (node/loop/mesh/surface)
- **Activation state** → Pulsing ring indicator

## Dependencies

```bash
# Python 3.7+
pip install Pillow
```

## Reproduce Demo

To reproduce the exact `renderer/demos/demo.gif`:

```bash
# 1. Install dependencies
pip install Pillow

# 2. Run the demo
bash demos/end_to_end_demo.sh

# 3. Verify output
ls -lh renderer/demos/demo.gif
```

Expected: GIF image data, 800x800, ~24KB

## Usage

```bash
python3 renderer/render.py <glyph_json_file> \
  --out output.gif \
  --duration 12 \
  --fps 12 \
  --zoom-levels "1,4,16"
```

See render.py for full API documentation.

**Version:** 1.0
