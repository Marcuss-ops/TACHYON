#!/usr/bin/env python3
"""Validate that the JIT authoring command does not link heavy engine modules."""

from __future__ import annotations

from pathlib import Path


REPO_ROOT = Path(__file__).resolve().parents[1]
TARGET = REPO_ROOT / "src" / "core" / "spec" / "authoring_service.cpp"

FORBIDDEN_LIBS = [
    "TachyonRenderer2D",
    "TachyonRuntime",
    "TachyonRuntimeEngine",
    "TachyonMedia",
    "TachyonOutput",
    "TachyonText",
    "TachyonPresets",
    "TachyonLibrary",
    "TachyonAudio",
    "TachyonColor",
    "TachyonPlatform",
]

ALLOWED_LIBS = [
    "TachyonScene",
]


def main() -> int:
    if not TARGET.exists():
        print(f"SKIP: {TARGET.relative_to(REPO_ROOT).as_posix()} not found")
        return 0

    text = TARGET.read_text(encoding="utf-8", errors="ignore")
    violations = []

    for lib in FORBIDDEN_LIBS:
        if f'"{lib}"' in text:
            violations.append(lib)

    if '"TachyonScene"' not in text:
        violations.append("missing allowed library: TachyonScene")

    if violations:
        print("FAIL: JIT still links legacy Tachyon libraries.")
        for lib in violations:
            print(f"  {lib}")
        print()
        print("The JIT authoring command should link only the minimal scene surface.")
        return 1

    print("OK: JIT link dependencies are minimal.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
