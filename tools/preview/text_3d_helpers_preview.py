#!/usr/bin/env python3

from __future__ import annotations

import argparse
import math
import os
import subprocess
import tempfile
from pathlib import Path

try:
    from PIL import Image, ImageDraw, ImageFont
except ImportError as exc:  # pragma: no cover - developer tool fallback
    raise SystemExit("Pillow is required for this preview tool. Install it with `pip install pillow`.") from exc


WINDOWS_FONT_CANDIDATES = [
    r"C:\Windows\Fonts\segoeuib.ttf",
    r"C:\Windows\Fonts\segoeui.ttf",
    r"C:\Windows\Fonts\arial.ttf",
    r"C:\Windows\Fonts\calibri.ttf",
]


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Lightweight text + 3D helpers preview renderer.")
    parser.add_argument("--text", default="TEXT + 3D HELPERS", help="Text to render.")
    parser.add_argument("--out", default="output/text_3d_helpers_preview.mp4", help="MP4 output path.")
    parser.add_argument("--frame-out", default="output/text_3d_helpers_preview_frame.png", help="Preview PNG output path.")
    parser.add_argument("--width", type=int, default=1280)
    parser.add_argument("--height", type=int, default=720)
    parser.add_argument("--fps", type=int, default=24)
    parser.add_argument("--duration", type=float, default=5.0)
    parser.add_argument("--preview-at", type=float, default=1.0, help="Time in seconds for the preview PNG.")
    return parser.parse_args()


def resolve_font(size: int) -> ImageFont.FreeTypeFont | ImageFont.ImageFont:
    override = os.environ.get("TACHYON_PREVIEW_FONT")
    if override:
        candidate = Path(override)
        if candidate.exists():
            try:
                return ImageFont.truetype(str(candidate), size=size)
            except OSError:
                pass

    for candidate in WINDOWS_FONT_CANDIDATES:
        if Path(candidate).exists():
            try:
                return ImageFont.truetype(candidate, size=size)
            except OSError:
                continue
    return ImageFont.load_default()


def draw_gradient_background(width: int, height: int) -> Image.Image:
    base = Image.new("RGBA", (width, height), (16, 18, 28, 255))
    px = base.load()
    cx = width * 0.52
    cy = height * 0.44
    max_dist = math.hypot(width, height)
    for y in range(height):
        for x in range(width):
            dx = x - cx
            dy = y - cy
            dist = math.hypot(dx, dy)
            glow = max(0.0, 1.0 - dist / max_dist)
            r = int(16 + glow * 18)
            g = int(18 + glow * 16)
            b = int(28 + glow * 30)
            px[x, y] = (r, g, b, 255)
    return base


def render_frame(width: int, height: int, text: str, t: float, duration: float) -> Image.Image:
    progress = max(0.0, min(1.0, t / max(0.001, duration)))
    entrance = max(0.0, min(1.0, progress / 0.18))
    tilt_y = 4.0 + math.sin(progress * math.tau * 0.65) * 4.0
    drift_x = math.sin(progress * math.tau * 0.30) * 16.0
    drift_y = math.sin(progress * math.tau * 0.22) * 5.0
    angle = tilt_y * 0.65

    frame = draw_gradient_background(width, height)
    title_font = resolve_font(72)
    text_layer = Image.new("RGBA", (width, height), (0, 0, 0, 0))
    layer_draw = ImageDraw.Draw(text_layer)

    title = text
    title_box = layer_draw.textbbox((0, 0), title, font=title_font)
    title_w = title_box[2] - title_box[0]
    title_h = title_box[3] - title_box[1]
    title_x = int(width * 0.5 - title_w / 2 + drift_x)
    title_y = int(height * 0.5 - title_h / 2 + drift_y + (1.0 - entrance) * 20.0)
    title_alpha = int(255 * max(0.0, min(1.0, entrance + 0.10)))

    # Shadow for readability.
    layer_draw.text((title_x + 4, title_y + 4), title, font=title_font, fill=(0, 0, 0, 100))
    layer_draw.text((title_x, title_y), title, font=title_font, fill=(255, 255, 255, title_alpha))

    rotated = text_layer.rotate(angle, resample=Image.Resampling.BICUBIC, expand=False)
    frame = Image.alpha_composite(frame, rotated)
    return frame


def encode_frames(frame_dir: Path, out_path: Path, fps: int) -> None:
    out_path.parent.mkdir(parents=True, exist_ok=True)
    pattern = str(frame_dir / "frame_%04d.png")
    cmd = [
        "ffmpeg",
        "-y",
        "-framerate",
        str(fps),
        "-i",
        pattern,
        "-c:v",
        "libx264",
        "-pix_fmt",
        "yuv420p",
        str(out_path),
    ]
    subprocess.run(cmd, check=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)


def main() -> int:
    args = parse_args()
    out_path = Path(args.out)
    frame_out = Path(args.frame_out)
    out_path.parent.mkdir(parents=True, exist_ok=True)
    frame_out.parent.mkdir(parents=True, exist_ok=True)

    with tempfile.TemporaryDirectory(prefix="tachyon_text_3d_") as tmpdir:
        frame_dir = Path(tmpdir)
        frame_count = max(1, int(round(args.duration * args.fps)))
        preview_index = min(frame_count - 1, max(0, int(round(args.preview_at * args.fps))))

        for index in range(frame_count):
            t = index / float(args.fps)
            frame = render_frame(args.width, args.height, args.text, t, args.duration)
            frame.save(frame_dir / f"frame_{index:04d}.png")
            if index == preview_index:
                frame.save(frame_out)

        encode_frames(frame_dir, out_path, args.fps)

    print(out_path)
    print(frame_out)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
