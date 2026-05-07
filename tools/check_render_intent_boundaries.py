#!/usr/bin/env python3
"""Check that render-intent headers stay out of core/presets domains."""

from __future__ import annotations

import re
import sys
from pathlib import Path


REPO_ROOT = Path(__file__).resolve().parents[1]
FORBIDDEN_ROOTS = [
    REPO_ROOT / "include" / "tachyon" / "core",
    REPO_ROOT / "src" / "core",
    REPO_ROOT / "include" / "tachyon" / "presets",
    REPO_ROOT / "src" / "presets",
]
INCLUDE_PATTERN = re.compile(r'^\s*#include\s+["<]tachyon/render/')


def iter_source_files(root: Path):
    if not root.exists():
        return
    yield from root.rglob("*.h")
    yield from root.rglob("*.hpp")
    yield from root.rglob("*.cpp")


def main() -> int:
    violations: list[str] = []

    for root in FORBIDDEN_ROOTS:
        for path in iter_source_files(root):
            try:
                text = path.read_text(encoding="utf-8")
            except UnicodeDecodeError:
                text = path.read_text(encoding="utf-8", errors="ignore")

            for line in text.splitlines():
                if INCLUDE_PATTERN.match(line):
                    rel = path.relative_to(REPO_ROOT).as_posix()
                    violations.append(f"{rel}: {line.strip()}")
                    break

    if violations:
        print("FAIL: RenderIntent boundary violations found.")
        for item in violations:
            print(f"  {item}")
        return 1

    print("OK: RenderIntent boundary respected.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
