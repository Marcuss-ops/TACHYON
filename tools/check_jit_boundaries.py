#!/usr/bin/env python3
"""Check JIT boundary rules.

This is a narrow architecture check for the JIT authoring/plugin boundary.
It enforces that JIT-facing code stays in the dedicated JIT SDK surface and
does not pull renderer/runtime internals across the DLL boundary.
"""

from __future__ import annotations

import re
from dataclasses import dataclass
from pathlib import Path


REPO_ROOT = Path(__file__).resolve().parents[1]
INCLUDE_PATTERN = re.compile(r'^\s*#include\s+["<](tachyon/[^">]+)[">]')
SOURCE_SUFFIXES = {".h", ".hpp", ".hh", ".cpp", ".cc", ".cxx"}


@dataclass(frozen=True)
class BoundaryRule:
    name: str
    roots: tuple[Path, ...]
    forbidden_prefixes: tuple[str, ...]


RULES = (
    BoundaryRule(
        name="jit_sdk_is_engine_light",
        roots=(
            REPO_ROOT / "include" / "tachyon" / "jit",
            REPO_ROOT / "src" / "jit",
        ),
        forbidden_prefixes=(
            "tachyon/renderer2d/",
            "tachyon/renderer3d/",
            "tachyon/runtime/",
            "tachyon/media/",
            "tachyon/output/",
            "tachyon/text/",
            "tachyon/background_registry.h",
            "tachyon/backgrounds.hpp",
            "tachyon/presets/",
        ),
    ),
)


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

                for line_no, line in enumerate(text.splitlines(), start=1):
                    match = INCLUDE_PATTERN.match(line)
                    if not match:
                        continue

                    include_path = match.group(1)
                    if any(
                        include_path == prefix or include_path.startswith(prefix)
                        for prefix in rule.forbidden_prefixes
                    ):
                        rel = path.relative_to(REPO_ROOT).as_posix()
                        violations.append(f"[{rule.name}] {rel}:{line_no}: {include_path}")

    if violations:
        print("FAIL: JIT architecture boundary violations found.")
        for item in violations:
            print(f"  {item}")
        return 1

    print("OK: JIT architecture boundaries respected.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
