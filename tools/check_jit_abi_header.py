#!/usr/bin/env python3
"""Validate that the public JIT ABI header stays C-ABI clean."""

from __future__ import annotations

from pathlib import Path


REPO_ROOT = Path(__file__).resolve().parents[1]
API_HEADER = REPO_ROOT / "include" / "tachyon" / "jit" / "tachyon_jit_api.h"

FORBIDDEN_TOKENS = [
    "std::",
    "std::vector",
    "std::string",
    "std::function",
    "std::shared_ptr",
    "std::unique_ptr",
    "template<",
    "template <",
    "class ",
    "namespace ",
    "BackgroundRegistry",
    "SceneBuilder",
    "BackgroundDescriptor",
]

REQUIRED_TOKENS = [
    "extern \"C\"",
    "TACHYON_JIT_ABI_VERSION",
    "struct TachyonHostApi",
    "struct TachyonJitManifest",
]


def main() -> int:
    if not API_HEADER.exists():
        print(f"FAIL: missing {API_HEADER.relative_to(REPO_ROOT).as_posix()}")
        return 1

    text = API_HEADER.read_text(encoding="utf-8", errors="ignore")
    violations = []

    for token in FORBIDDEN_TOKENS:
        if token in text:
            violations.append(f"forbidden token: {token}")

    for token in REQUIRED_TOKENS:
        if token not in text:
            violations.append(f"missing required token: {token}")

    if violations:
        print("FAIL: JIT ABI header is not C-ABI clean.")
        for item in violations:
            print(f"  {item}")
        return 1

    print("OK: JIT ABI header is C-ABI clean.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
