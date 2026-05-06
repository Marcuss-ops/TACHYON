#!/usr/bin/env python3

from __future__ import annotations

import argparse
import math
import os
import re
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

TYPEWRITER_PACK_VARIANTS = [
    {
        "id": "tachyon.textanim.typewriter",
        "title": "TYPEWRITER",
        "subtitle": "Classic character reveal",
        "style": "classic",
        "reveal_mode": "characters",
        "text": "TYPEWRITER PACK",
        "chars_per_second": 18.0,
        "cursor": True,
        "cursor_blink_hz": 2.0,
        "cursor_opacity": 0.72,
    },
    {
        "id": "tachyon.textanim.typewriter.classic",
        "title": "CLASSIC",
        "subtitle": "Balanced baseline",
        "style": "classic",
        "reveal_mode": "characters",
        "text": "THE FIRST WARNING",
        "chars_per_second": 16.0,
        "cursor": True,
        "cursor_blink_hz": 2.5,
        "cursor_opacity": 0.68,
    },
    {
        "id": "tachyon.textanim.typewriter.cursor",
        "title": "CURSOR",
        "subtitle": "Stronger blink and tighter pace",
        "style": "classic",
        "reveal_mode": "characters",
        "text": "OPEN THE FILE",
        "chars_per_second": 19.0,
        "cursor": True,
        "cursor_blink_hz": 4.0,
        "cursor_opacity": 0.88,
    },
    {
        "id": "tachyon.textanim.typewriter.soft",
        "title": "SOFT",
        "subtitle": "Gentler fade settle",
        "style": "archive",
        "reveal_mode": "characters",
        "text": "REDACTED NOTES",
        "chars_per_second": 15.0,
        "cursor": False,
        "cursor_blink_hz": 0.0,
        "cursor_opacity": 0.0,
    },
    {
        "id": "tachyon.textanim.typewriter.word",
        "title": "WORD",
        "subtitle": "Word-by-word rhythm",
        "style": "classic",
        "reveal_mode": "words",
        "text": "HE DISAPPEARED WITHOUT A TRACE",
        "chars_per_second": 4.0,
        "cursor": False,
        "cursor_blink_hz": 0.0,
        "cursor_opacity": 0.0,
    },
    {
        "id": "tachyon.textanim.typewriter.word_cursor",
        "title": "WORD + CURSOR",
        "subtitle": "Word reveal with cursor",
        "style": "classic",
        "reveal_mode": "words",
        "text": "NEW EVIDENCE APPEARS HERE",
        "chars_per_second": 3.5,
        "cursor": True,
        "cursor_blink_hz": 3.0,
        "cursor_opacity": 0.76,
    },
    {
        "id": "tachyon.textanim.typewriter.line",
        "title": "LINE",
        "subtitle": "Line-by-line reveal",
        "style": "archive",
        "reveal_mode": "lines",
        "text": "CASE FILE 07\nOPEN BY REQUEST\nHANDLE WITH CARE",
        "chars_per_second": 2.0,
        "cursor": False,
        "cursor_blink_hz": 0.0,
        "cursor_opacity": 0.0,
    },
    {
        "id": "tachyon.textanim.typewriter.sentence",
        "title": "SENTENCE",
        "subtitle": "Phrase-style reveal",
        "style": "classic",
        "reveal_mode": "words",
        "text": "THE STORY BREAKS HERE",
        "chars_per_second": 2.4,
        "cursor": True,
        "cursor_blink_hz": 2.2,
        "cursor_opacity": 0.68,
    },
    {
        "id": "tachyon.textanim.typewriter.archive",
        "title": "ARCHIVE",
        "subtitle": "Muted documentation tone",
        "style": "archive",
        "reveal_mode": "characters",
        "text": "ARCHIVE ENTRY 1994",
        "chars_per_second": 15.0,
        "cursor": False,
        "cursor_blink_hz": 0.0,
        "cursor_opacity": 0.0,
    },
    {
        "id": "tachyon.textanim.typewriter.terminal",
        "title": "TERMINAL",
        "subtitle": "Console green output",
        "style": "terminal",
        "reveal_mode": "characters",
        "text": "RUNNING FORENSIC CHECK",
        "chars_per_second": 18.0,
        "cursor": True,
        "cursor_blink_hz": 3.5,
        "cursor_opacity": 0.88,
    },
]


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Lightweight typewriter preview renderer.")
    parser.add_argument("--text", default="TYPEWRITER PACK", help="Text to render.")
    parser.add_argument("--style", choices=("classic", "terminal", "archive"), default="classic")
    parser.add_argument("--out", default="output/typewriter_preview.mp4", help="MP4 output path.")
    parser.add_argument("--frame-out", default="output/typewriter_preview_frame.png", help="Preview PNG output path.")
    parser.add_argument("--batch-out", default="", help="Optional directory for one MP4 per typewriter preset.")
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


def count_units(text: str, reveal_mode: str) -> int:
    if reveal_mode == "lines":
        return max(1, len(text.splitlines()))
    if reveal_mode == "words":
        return max(1, len([token for token in re.findall(r"\S+", text) if token.strip()]))
    return max(1, len(text))


def build_visible_text(text: str, reveal_mode: str, progress: float) -> str:
    if reveal_mode == "lines":
        lines = text.splitlines()
        count = max(0, min(len(lines), int(progress)))
        return "\n".join(lines[:count])
    if reveal_mode == "words":
        tokens = re.findall(r"\s+|[^\s]+", text)
        visible = []
        word_count = 0
        for token in tokens:
            if token.isspace():
                if word_count > 0 and word_count <= int(progress):
                    visible.append(token)
                continue
            if word_count >= int(progress):
                break
            visible.append(token)
            word_count += 1
        return "".join(visible).rstrip()
    count = max(0, min(len(text), int(progress)))
    return text[:count]


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
    reveal_mode: str = "characters",
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
    visible_text = build_visible_text(text, reveal_mode, visible_count)
    total_units = float(count_units(text, reveal_mode))

    block_width, block_height = block_metrics(draw, font, visible_text or " ")
    x = float((width - block_width) / 2.0)
    y = float((height - block_height) / 2.0)
    lh = line_height(font)

    if visible_text:
        text_draw.multiline_text((x, y), visible_text, font=font, fill=foreground, spacing=12)
    image.alpha_composite(text_layer)

    cursor_progress = visible_count
    if reveal_mode == "characters":
        total_units = float(len(text))
    elif reveal_mode == "lines":
        total_units = float(max(1, len(text.splitlines())))
    else:
        total_units = float(count_units(text, reveal_mode))

    if show_cursor and cursor_progress < total_units:
        blink = math.sin(2.0 * math.pi * cursor_blink_hz * elapsed)
        cursor_alpha = max(0.0, min(1.0, cursor_opacity * (0.5 + 0.5 * blink)))
        if cursor_alpha > 0.0:
            cursor_rgba = (
                cursor_color[0],
                cursor_color[1],
                cursor_color[2],
                max(0, min(255, int(255 * cursor_alpha))),
            )
            visible_lines = visible_text.splitlines() if visible_text else [""]
            cursor_line = visible_lines[-1] if visible_lines else ""
            cursor_x = x + glyph_width(draw, font, cursor_line)
            cursor_y = y + (len(visible_lines) - 1) * lh
            cursor_w = max(4, int(glyph_width(draw, font, "|")))
            cursor_h = max(line_height(font) - 10, int(font.size if hasattr(font, "size") else 36))
            draw.rectangle(
                [cursor_x + 2, cursor_y - 4, cursor_x + 2 + cursor_w, cursor_y - 4 + cursor_h],
                fill=cursor_rgba,
            )

    return image


def render_variant_gallery(variants: list[dict[str, object]], out_path: Path, width: int, height: int, font: ImageFont.ImageFont) -> None:
    card_w = 920
    card_h = 300
    thumb_w = 274
    thumb_h = 150
    gap_x = 26
    gap_y = 26
    padding = 28
    columns = 2
    rows = math.ceil(len(variants) / columns)
    gallery_w = columns * card_w + (columns - 1) * gap_x + padding * 2
    gallery_h = rows * card_h + (rows - 1) * gap_y + padding * 2

    sheet = Image.new("RGBA", (gallery_w, gallery_h), (9, 12, 18, 255))
    draw = ImageDraw.Draw(sheet)

    title_font = resolve_font("classic", 36)
    subtitle_font = resolve_font("classic", 20)
    meta_font = resolve_font("terminal", 18)

    sample_times = (0.15, 0.85, 1.8)

    for index, variant in enumerate(variants):
        row = index // columns
        col = index % columns
        x0 = padding + col * (card_w + gap_x)
        y0 = padding + row * (card_h + gap_y)

        card = Image.new("RGBA", (card_w, card_h), (15, 19, 28, 255))
        card_draw = ImageDraw.Draw(card)
        card_draw.rounded_rectangle([0, 0, card_w - 1, card_h - 1], radius=22, fill=(15, 19, 28, 255), outline=(55, 67, 88, 255), width=2)

        title = str(variant["title"])
        subtitle = str(variant["subtitle"])
        preset_id = str(variant["id"])

        card_draw.text((24, 18), title, font=title_font, fill=(245, 248, 255, 255))
        card_draw.text((24, 58), subtitle, font=subtitle_font, fill=(168, 178, 198, 255))
        card_draw.text((24, 84), preset_id, font=meta_font, fill=(95, 165, 255, 255))

        text = str(variant["text"])
        style = str(variant["style"])
        variant_font_size = 68 if style == "terminal" else 72
        variant_font = resolve_font(style, variant_font_size)
        cps = float(variant["chars_per_second"])
        cursor = bool(variant["cursor"])
        blink_hz = float(variant["cursor_blink_hz"])
        cursor_opacity = float(variant["cursor_opacity"])
        reveal_mode = str(variant["reveal_mode"])

        thumb_y = 116
        for snap_index, snap_time in enumerate(sample_times):
            frame = render_frame(
                text,
                style,
                thumb_w,
                thumb_h,
                variant_font,
                snap_time,
                cps,
                cursor,
                blink_hz,
                cursor_opacity,
                reveal_mode=reveal_mode,
            )
            thumb_x = 24 + snap_index * (thumb_w + 12)
            card.alpha_composite(frame, (thumb_x, thumb_y))
            card_draw.rounded_rectangle([thumb_x, thumb_y, thumb_x + thumb_w, thumb_y + thumb_h], radius=12, outline=(40, 50, 68, 255), width=2)

        sheet.alpha_composite(card, (x0, y0))

    out_path.parent.mkdir(parents=True, exist_ok=True)
    sheet.save(out_path)


def sanitize_filename(value: str) -> str:
    safe = value.split(".")[-1] if "." in value else value
    safe = re.sub(r"[^A-Za-z0-9_-]+", "_", safe)
    return safe.strip("_") or "variant"


def render_preview_video(
    ffmpeg: str,
    out_path: Path,
    frame_out: Path | None,
    text: str,
    style: str,
    width: int,
    height: int,
    font: ImageFont.ImageFont,
    duration: float,
    fps: int,
    chars_per_second: float,
    show_cursor: bool,
    cursor_blink_hz: float,
    cursor_opacity: float,
    preview_at: float,
    reveal_mode: str = "characters",
) -> None:
    total_frames = max(1, int(math.ceil(duration * fps)))
    preview_frame = max(0, min(total_frames - 1, int(round(preview_at * fps))))

    out_path.parent.mkdir(parents=True, exist_ok=True)
    if frame_out is not None:
        frame_out.parent.mkdir(parents=True, exist_ok=True)

    with tempfile.TemporaryDirectory(prefix="tachyon_typewriter_preview_") as tmp_dir:
        tmp_dir_path = Path(tmp_dir)
        for frame_index in range(total_frames):
            elapsed = frame_index / float(fps)
            frame = render_frame(
                text,
                style,
                width,
                height,
                font,
                elapsed,
                chars_per_second,
                show_cursor,
                cursor_blink_hz,
                cursor_opacity,
                reveal_mode=reveal_mode,
            )
            frame_path = tmp_dir_path / f"frame_{frame_index:06d}.png"
            frame.save(frame_path)
            if frame_out is not None and frame_index == preview_frame:
                frame.save(frame_out)

        cmd = [
            ffmpeg,
            "-y",
            "-hide_banner",
            "-loglevel",
            "error",
            "-framerate",
            str(fps),
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
    font_size = 120 if args.style != "terminal" else 108
    font = resolve_font(args.style, font_size)

    if args.batch_out:
        batch_dir = Path(args.batch_out)
        batch_dir.mkdir(parents=True, exist_ok=True)
        for variant in TYPEWRITER_PACK_VARIANTS:
            variant_font_size = 68 if variant["style"] == "terminal" else 72
            variant_font = resolve_font(str(variant["style"]), variant_font_size)
            variant_name = sanitize_filename(str(variant["id"]))
            variant_out = batch_dir / f"{variant_name}.mp4"
            render_preview_video(
                ffmpeg,
                variant_out,
                None,
                str(variant["text"]),
                str(variant["style"]),
                args.width,
                args.height,
                variant_font,
                args.duration,
                args.fps,
                float(variant["chars_per_second"]),
                bool(variant["cursor"]),
                float(variant["cursor_blink_hz"]),
                float(variant["cursor_opacity"]),
                args.preview_at,
                reveal_mode=str(variant["reveal_mode"]),
            )
            print(f"wrote {variant_out}")
        return 0

    render_preview_video(
        ffmpeg,
        out_path,
        frame_out,
        args.text,
        args.style,
        args.width,
        args.height,
        font,
        args.duration,
        args.fps,
        args.chars_per_second,
        args.cursor,
        args.cursor_blink_hz,
        args.cursor_opacity,
        args.preview_at,
    )

    print(f"wrote {out_path}")
    print(f"wrote {frame_out}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
