#!/usr/bin/env python3
"""Check high-level graphics architecture boundaries.

This script enforces that neutral spec/preset/scene-eval code does not
directly depend on renderer-specific headers or render-intent headers.
"""

from __future__ import annotations

import re
import sys
from dataclasses import dataclass
from pathlib import Path


REPO_ROOT = Path(__file__).resolve().parents[1]
INCLUDE_PATTERN = re.compile(r'^\s*#include\s+["<](tachyon/[^">]+)[">]')


@dataclass(frozen=True)
class BoundaryRule:
    name: str
    roots: tuple[Path, ...]
    forbidden_prefixes: tuple[str, ...]


RULES = (
    BoundaryRule(
        name="core_spec_is_renderer_neutral",
        roots=(
            REPO_ROOT / "include" / "tachyon" / "core" / "spec",
            REPO_ROOT / "src" / "core" / "spec",
        ),
        forbidden_prefixes=(
            "tachyon/renderer2d/",
            "tachyon/renderer3d/",
            "tachyon/render/render_intent.h",
            "tachyon/render/intent_builder.h",
        ),
    ),
    BoundaryRule(
        name="presets_are_renderer_neutral",
        roots=(
            REPO_ROOT / "include" / "tachyon" / "presets",
            REPO_ROOT / "src" / "presets",
        ),
        forbidden_prefixes=(
            "tachyon/renderer2d/",
            "tachyon/renderer3d/",
            "tachyon/render/render_intent.h",
            "tachyon/render/intent_builder.h",
        ),
    ),
    BoundaryRule(
        name="scene_eval_is_renderer_neutral",
        roots=(
            REPO_ROOT / "include" / "tachyon" / "core" / "scene",
            REPO_ROOT / "src" / "core" / "scene",
        ),
        forbidden_prefixes=(
            "tachyon/renderer2d/",
            "tachyon/renderer3d/",
            "tachyon/render/render_intent.h",
            "tachyon/render/intent_builder.h",
        ),
    ),
)

SOURCE_SUFFIXES = {".h", ".hpp", ".hh", ".cpp", ".cc", ".cxx"}


def iter_source_files(root: Path):
    if not root.exists():
        return
    for path in root.rglob("*"):
        if path.is_file() and path.suffix in SOURCE_SUFFIXES:
            yield path


def main() -> int:
    violations: list[str] = []

    for rule in RULES:
        for root in rule.roots:
            for path in iter_source_files(root):
                try:
                    text = path.read_text(encoding="utf-8")
                except UnicodeDecodeError:
                    text = path.read_text(encoding="utf-8", errors="ignore")

                for line in text.splitlines():
                    match = INCLUDE_PATTERN.match(line)
                    if not match:
                        continue

                    include_path = match.group(1)
                    if any(
                        include_path == prefix or include_path.startswith(prefix)
                        for prefix in rule.forbidden_prefixes
                    ):
                        rel = path.relative_to(REPO_ROOT).as_posix()
                        violations.append(f"[{rule.name}] {rel}: {line.strip()}")
                        break

    if violations:
        print("FAIL: graphics architecture boundary violations found.")
        for item in violations:
            print(f"  {item}")
        return 1

    print("OK: graphics architecture boundaries respected.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
