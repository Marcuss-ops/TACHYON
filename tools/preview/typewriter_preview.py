#!/usr/bin/env python3

from __future__ import annotations

import argparse
import math
import os
import shutil
import subprocess
import sys
import tempfile
from pathlib import Path

try:
    from PIL import Image, ImageDraw, ImageFont
except ImportError as exc:  # pragma: no cover - developer tool fallback
    raise SystemExit("Pillow is required for this preview tool. Install it with `pip install pillow`.") from exc


WINDOWS_FONT_CANDIDATES = {
    "classic": [
        r"C:\Windows\Fonts\segoeuib.ttf",
        r"C:\Windows\Fonts\segoeui.ttf",
        r"C:\Windows\Fonts\arial.ttf",
        r"C:\Windows\Fonts\calibri.ttf",
    ],
    "terminal": [
        r"C:\Windows\Fonts\consola.ttf",
        r"C:\Windows\Fonts\lucon.ttf",
        r"C:\Windows\Fonts\cour.ttf",
    ],
    "archive": [
        r"C:\Windows\Fonts\times.ttf",
        r"C:\Windows\Fonts\georgia.ttf",
        r"C:\Windows\Fonts\arial.ttf",
    ],
}


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Lightweight typewriter preview renderer.")
    parser.add_argument("--text", default="TYPEWRITER PACK", help="Text to render.")
    parser.add_argument("--style", choices=("classic", "terminal", "archive"), default="classic")
    parser.add_argument("--out", default="output/typewriter_preview.mp4", help="MP4 output path.")
    parser.add_argument("--frame-out", default="output/typewriter_preview_frame.png", help="Preview PNG output path.")
    parser.add_argument("--width", type=int, default=1920)
    parser.add_argument("--height", type=int, default=1080)
    parser.add_argument("--fps", type=int, default=24)
    parser.add_argument("--duration", type=float, default=4.0)
    parser.add_argument("--chars-per-second", type=float, default=18.0)
    parser.add_argument("--preview-at", type=float, default=1.0, help="Time in seconds for the preview PNG.")
    parser.add_argument("--cursor", action="store_true", default=True, help="Show the cursor.")
    parser.add_argument("--no-cursor", dest="cursor", action="store_false", help="Hide the cursor.")
    parser.add_argument("--cursor-blink-hz", type=float, default=2.0)
    parser.add_argument("--cursor-opacity", type=float, default=0.75)
    return parser.parse_args()


def resolve_font(style: str, size: int) -> ImageFont.FreeTypeFont | ImageFont.ImageFont:
    override = os.environ.get("TACHYON_PREVIEW_FONT")
    candidates = [override] if override else []
    candidates.extend(WINDOWS_FONT_CANDIDATES.get(style, []))
    candidates.extend([
        r"C:\Windows\Fonts\arial.ttf",
        r"C:\Windows\Fonts\consola.ttf",
        r"C:\Windows\Fonts\segoeui.ttf",
    ])
    for candidate in candidates:
        if candidate and Path(candidate).exists():
            try:
                return ImageFont.truetype(candidate, size=size)
            except OSError:
                continue
    return ImageFont.load_default()


def theme(style: str) -> dict[str, tuple[int, int, int, int] | int]:
    if style == "terminal":
        return {
            "background": (4, 12, 8, 255),
            "foreground": (0, 255, 128, 255),
            "cursor": (0, 255, 128, 255),
        }
    if style == "archive":
        return {
            "background": (18, 18, 21, 255),
            "foreground": (223, 225, 232, 255),
            "cursor": (223, 225, 232, 255),
        }
    return {
        "background": (11, 15, 20, 255),
        "foreground": (245, 248, 255, 255),
        "cursor": (245, 248, 255, 255),
    }


def line_height(font: ImageFont.ImageFont) -> int:
    ascent, descent = font.getmetrics() if hasattr(font, "getmetrics") else (12, 4)
    return int(ascent + descent + 12)


def glyph_width(draw: ImageDraw.ImageDraw, font: ImageFont.ImageFont, text: str) -> float:
    if not text:
        return 0.0
    try:
        return draw.textlength(text, font=font)
    except Exception:
        bbox = draw.textbbox((0, 0), text, font=font)
        return float(bbox[2] - bbox[0])


def block_metrics(draw: ImageDraw.ImageDraw, font: ImageFont.ImageFont, text: str) -> tuple[int, int]:
    lines = text.splitlines() or [text]
    widths = [glyph_width(draw, font, line) for line in lines]
    block_width = int(max(widths) if widths else 0)
    block_height = int(len(lines) * line_height(font) - 12 if lines else line_height(font))
    return block_width, block_height


def render_frame(
    text: str,
    style: str,
    width: int,
    height: int,
    font: ImageFont.ImageFont,
    elapsed: float,
    chars_per_second: float,
    show_cursor: bool,
    cursor_blink_hz: float,
    cursor_opacity: float,
) -> Image.Image:
    t = theme(style)
    background = t["background"]
    foreground = t["foreground"]
    cursor_color = t["cursor"]

    image = Image.new("RGBA", (width, height), background)
    draw = ImageDraw.Draw(image)
    text_layer = Image.new("RGBA", (width, height), (0, 0, 0, 0))
    text_draw = ImageDraw.Draw(text_layer)

    visible_count = max(0.0, elapsed * chars_per_second)
    cursor_progress = min(visible_count, float(len(text)))

    block_width, block_height = block_metrics(draw, font, text)
    x = float((width - block_width) / 2.0)
    y = float((height - block_height) / 2.0)
    start_x = x
    lh = line_height(font)

    cursor_x = x
    cursor_y = y
    processed = 0.0

    for ch in text:
        if ch == "\n":
            x = start_x
            y += lh
            cursor_x = x
            cursor_y = y
            if processed < visible_count:
                processed += 1.0
            continue

        w = glyph_width(draw, font, ch)
        alpha = 0.0
        if processed < visible_count:
            alpha = 1.0
        elif processed < visible_count + 1.0:
            alpha = max(0.0, min(1.0, visible_count - processed))

        if alpha > 0.0:
            rgba = (
                foreground[0],
                foreground[1],
                foreground[2],
                max(0, min(255, int(foreground[3] * alpha))),
            )
            text_draw.text((x, y), ch, font=font, fill=rgba)

        if processed <= cursor_progress:
            cursor_x = x + w
            cursor_y = y

        x += w
        processed += 1.0

    image.alpha_composite(text_layer)

    if show_cursor and cursor_progress < float(len(text)):
        blink = math.sin(2.0 * math.pi * cursor_blink_hz * elapsed)
        cursor_alpha = max(0.0, min(1.0, cursor_opacity * (0.5 + 0.5 * blink)))
        if cursor_alpha > 0.0:
            cursor_rgba = (
                cursor_color[0],
                cursor_color[1],
                cursor_color[2],
                max(0, min(255, int(255 * cursor_alpha))),
            )
            cursor_w = max(4, int(glyph_width(draw, font, "|")))
            cursor_h = max(line_height(font) - 10, int(font.size if hasattr(font, "size") else 36))
            draw.rectangle(
                [cursor_x + 2, cursor_y - 4, cursor_x + 2 + cursor_w, cursor_y - 4 + cursor_h],
                fill=cursor_rgba,
            )

    return image


def ensure_ffmpeg() -> str:
    exe = shutil.which("ffmpeg")
    if not exe:
        raise SystemExit("ffmpeg was not found in PATH.")
    return exe


def main() -> int:
    args = parse_args()
    ffmpeg = ensure_ffmpeg()

    out_path = Path(args.out)
    frame_out = Path(args.frame_out)
    out_path.parent.mkdir(parents=True, exist_ok=True)
    frame_out.parent.mkdir(parents=True, exist_ok=True)

    font_size = 120 if args.style != "terminal" else 108
    font = resolve_font(args.style, font_size)
    total_frames = max(1, int(math.ceil(args.duration * args.fps)))
    preview_frame = max(0, min(total_frames - 1, int(round(args.preview_at * args.fps))))

    with tempfile.TemporaryDirectory(prefix="tachyon_typewriter_preview_") as tmp_dir:
        tmp_dir_path = Path(tmp_dir)
        for frame_index in range(total_frames):
            elapsed = frame_index / float(args.fps)
            frame = render_frame(
                args.text,
                args.style,
                args.width,
                args.height,
                font,
                elapsed,
                args.chars_per_second,
                args.cursor,
                args.cursor_blink_hz,
                args.cursor_opacity,
            )
            frame_path = tmp_dir_path / f"frame_{frame_index:06d}.png"
            frame.save(frame_path)
            if frame_index == preview_frame:
                frame.save(frame_out)

        cmd = [
            ffmpeg,
            "-y",
            "-hide_banner",
            "-loglevel",
            "error",
            "-framerate",
            str(args.fps),
            "-i",
            str(tmp_dir_path / "frame_%06d.png"),
            "-c:v",
            "libx264",
            "-pix_fmt",
            "yuv420p",
            "-movflags",
            "+faststart",
            str(out_path),
        ]
        subprocess.run(cmd, check=True)

    print(f"wrote {out_path}")
    print(f"wrote {frame_out}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
