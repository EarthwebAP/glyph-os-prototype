#!/usr/bin/env python3
"""
render.py - Parametric renderer for glyphs

Reads persisted glyphs and renders visual representations based on:
- state.energy → size/brightness (linear mapping)
- resonance.tone.freq → color hue
- form.topology → base shape (node/edge → circle/line)
"""

import argparse
import colorsys
import json
import math
import sys
from pathlib import Path

try:
    from PIL import Image, ImageDraw
except ImportError:
    print("Error: PIL/Pillow required. Install: pip install Pillow", file=sys.stderr)
    sys.exit(1)


class GlyphRenderer:
    """Parametric renderer for glyphs"""

    def __init__(self, width=800, height=800, bg_color=(10, 10, 20)):
        self.width = width
        self.height = height
        self.bg_color = bg_color
        self.center_x = width // 2
        self.center_y = height // 2

    def load_glyph(self, glyph_path):
        """Load glyph from JSON file"""
        with open(glyph_path, 'r') as f:
            return json.load(f)

    def energy_to_size(self, energy, base_size=50, scale=10):
        """
        Map energy to size (linear)
        energy=0 → base_size
        energy=10 → base_size + scale*10
        """
        return base_size + (energy * scale)

    def energy_to_brightness(self, energy, max_energy=10.0):
        """
        Map energy to brightness multiplier (linear)
        energy=0 → 0.3
        energy=max → 1.0
        """
        normalized = min(energy / max_energy, 1.0)
        return 0.3 + (normalized * 0.7)

    def freq_to_hue(self, freq):
        """
        Map frequency to hue (0-360 degrees)
        20 Hz → 0° (red)
        440 Hz (A4) → 180° (cyan)
        20000 Hz → 360° (red again)
        """
        # Logarithmic mapping feels more natural for frequency
        min_freq = 20.0
        max_freq = 20000.0

        # Clamp frequency
        freq = max(min_freq, min(freq, max_freq))

        # Log scale
        log_freq = math.log(freq)
        log_min = math.log(min_freq)
        log_max = math.log(max_freq)

        normalized = (log_freq - log_min) / (log_max - log_min)
        hue = normalized * 360.0

        return hue

    def hue_to_rgb(self, hue, brightness=1.0):
        """Convert hue (0-360) to RGB (0-255)"""
        # Convert to 0-1 range
        h = (hue % 360) / 360.0
        s = 0.8  # High saturation for vibrant colors
        v = brightness

        r, g, b = colorsys.hsv_to_rgb(h, s, v)
        return (int(r * 255), int(g * 255), int(b * 255))

    def topology_to_shape(self, topology):
        """Map topology to base shapes"""
        if not topology:
            return "node"

        # Use first topology element
        first = topology[0].lower()

        shape_map = {
            "node": "circle",
            "edge": "line",
            "loop": "ring",
            "mesh": "polygon",
            "surface": "filled_polygon",
            "void": "void"
        }

        return shape_map.get(first, "circle")

    def draw_shape(self, draw, shape, x, y, size, color, complexity=3):
        """Draw a shape at the given position"""
        half_size = size / 2

        if shape == "circle":
            # Filled circle
            draw.ellipse(
                [x - half_size, y - half_size, x + half_size, y + half_size],
                fill=color,
                outline=color
            )
            # Glow effect
            for i in range(3):
                glow_size = half_size + (i + 1) * 5
                glow_alpha = 100 - (i * 30)
                glow_color = color + (glow_alpha,)
                draw.ellipse(
                    [x - glow_size, y - glow_size, x + glow_size, y + glow_size],
                    outline=glow_color,
                    width=2
                )

        elif shape == "ring":
            # Outer circle
            draw.ellipse(
                [x - half_size, y - half_size, x + half_size, y + half_size],
                outline=color,
                width=max(2, int(size / 20))
            )
            # Inner circle
            inner_size = half_size * 0.6
            draw.ellipse(
                [x - inner_size, y - inner_size, x + inner_size, y + inner_size],
                outline=color,
                width=max(1, int(size / 30))
            )

        elif shape == "line":
            # Draw cross
            draw.line(
                [x - half_size, y, x + half_size, y],
                fill=color,
                width=max(2, int(size / 15))
            )
            draw.line(
                [x, y - half_size, x, y + half_size],
                fill=color,
                width=max(2, int(size / 15))
            )

        elif shape == "polygon":
            # Regular polygon based on complexity
            points = []
            for i in range(complexity):
                angle = (i / complexity) * 2 * math.pi
                px = x + half_size * math.cos(angle)
                py = y + half_size * math.sin(angle)
                points.append((px, py))

            draw.polygon(points, outline=color, width=2)

        elif shape == "filled_polygon":
            # Filled polygon
            points = []
            for i in range(complexity):
                angle = (i / complexity) * 2 * math.pi
                px = x + half_size * math.cos(angle)
                py = y + half_size * math.sin(angle)
                points.append((px, py))

            draw.polygon(points, fill=color, outline=color)

        elif shape == "void":
            # Void: just draw outline
            draw.ellipse(
                [x - half_size, y - half_size, x + half_size, y + half_size],
                outline=color,
                width=1
            )

    def render_glyph(self, glyph, zoom=1.0):
        """Render a single glyph at given zoom level"""
        # Create image
        img = Image.new('RGBA', (self.width, self.height), self.bg_color + (255,))
        draw = ImageDraw.Draw(img)

        # Extract glyph properties
        energy = glyph.get('state', {}).get('energy', 0.0)
        activated = glyph.get('state', {}).get('activated', False)
        topology = glyph.get('form', {}).get('topology', ['node'])
        parameters = glyph.get('form', {}).get('parameters', {})
        resonance = glyph.get('resonance', {})

        # Get frequency (default to 440 Hz if not specified)
        freq = 440.0
        if resonance and 'tone' in resonance:
            freq = resonance['tone'].get('freq', 440.0)

        # Calculate visual properties
        base_size = self.energy_to_size(energy) * zoom
        brightness = self.energy_to_brightness(energy)
        hue = self.freq_to_hue(freq)
        color = self.hue_to_rgb(hue, brightness)
        shape = self.topology_to_shape(topology)
        complexity = parameters.get('complexity', 3)

        # Draw main shape
        self.draw_shape(
            draw,
            shape,
            self.center_x,
            self.center_y,
            base_size,
            color,
            complexity
        )

        # Add activation indicator if activated
        if activated:
            # Pulsing ring
            for i in range(3):
                pulse_size = base_size + 20 + (i * 10)
                pulse_alpha = 150 - (i * 40)
                draw.ellipse(
                    [
                        self.center_x - pulse_size,
                        self.center_y - pulse_size,
                        self.center_x + pulse_size,
                        self.center_y + pulse_size
                    ],
                    outline=color + (pulse_alpha,),
                    width=2
                )

        # Add text overlay
        info_text = f"Energy: {energy:.2f} | Freq: {freq:.0f}Hz | Zoom: {zoom:.0f}x"
        draw.text((10, 10), info_text, fill=(200, 200, 200))

        if activated:
            draw.text((10, 30), "ACTIVATED", fill=(0, 255, 0))

        return img

    def render_sequence(self, glyph, zoom_levels=[1.0, 4.0, 16.0], frames_per_zoom=4):
        """Render a sequence of frames at different zoom levels"""
        frames = []

        for zoom in zoom_levels:
            for _ in range(frames_per_zoom):
                frame = self.render_glyph(glyph, zoom)
                frames.append(frame)

        return frames

    def save_gif(self, frames, output_path, duration=100, loop=0):
        """Save frames as animated GIF"""
        if not frames:
            raise ValueError("No frames to save")

        frames[0].save(
            output_path,
            save_all=True,
            append_images=frames[1:],
            duration=duration,
            loop=loop,
            optimize=False
        )


def main():
    parser = argparse.ArgumentParser(description="Render glyph to GIF")
    parser.add_argument("glyph_path", help="Path to glyph JSON file")
    parser.add_argument("--out", default="output.gif", help="Output GIF path")
    parser.add_argument("--duration", type=int, default=12, help="Total duration in seconds")
    parser.add_argument("--fps", type=int, default=12, help="Frames per second")
    parser.add_argument("--width", type=int, default=800, help="Image width")
    parser.add_argument("--height", type=int, default=800, help="Image height")
    parser.add_argument("--zoom-levels", type=str, default="1,4,16",
                        help="Comma-separated zoom levels")

    args = parser.parse_args()

    # Parse zoom levels
    zoom_levels = [float(z.strip()) for z in args.zoom_levels.split(',')]

    # Calculate frames
    total_frames = args.duration * args.fps
    frames_per_zoom = total_frames // len(zoom_levels)
    frame_duration = 1000 // args.fps  # milliseconds

    # Create renderer
    renderer = GlyphRenderer(width=args.width, height=args.height)

    # Load glyph
    print(f"Loading glyph from: {args.glyph_path}")
    glyph = renderer.load_glyph(args.glyph_path)

    # Render sequence
    print(f"Rendering {total_frames} frames at zoom levels: {zoom_levels}")
    frames = renderer.render_sequence(glyph, zoom_levels, frames_per_zoom)

    # Save GIF
    print(f"Saving GIF to: {args.out}")
    renderer.save_gif(frames, args.out, duration=frame_duration, loop=0)

    print(f"Done! Created {len(frames)} frames")
    print(f"GIF duration: {args.duration}s at {args.fps} fps")

    return 0


if __name__ == "__main__":
    sys.exit(main())
