"""
tachyon.py — Python bindings for libtachyon via ctypes.

Usage:
    from tachyon import Tachyon

    engine = Tachyon()                          # finds libtachyon.so automatically
    engine.render("scene.cpp", "output.mp4")
    engine.thumb("scene.cpp", "thumb.png", frame_time=0.5)
    engine.probe("video.mp4")                   # returns stdout as str

Build the shared library first:
    cmake --preset dev-linux -DTACHYON_BUILD_SHARED_LIB=ON
    cmake --build build/dev-linux --target tachyon_lib
"""

from __future__ import annotations

import ctypes
import os
import subprocess
import sys
from pathlib import Path
from typing import Optional, Sequence


def _find_libtachyon(hint: Optional[str] = None) -> ctypes.CDLL:
    candidates = []
    if hint:
        candidates.append(hint)

    # Look next to this file, then in common build dirs
    here = Path(__file__).parent
    for d in [here, here.parent.parent / "build" / "dev-linux" / "src",
              here.parent.parent / "build" / "release" / "src"]:
        for name in ["libtachyon.so", "tachyon.dll", "libtachyon.dylib"]:
            candidates.append(str(d / name))

    for path in candidates:
        if Path(path).exists():
            return ctypes.CDLL(path)

    raise FileNotFoundError(
        "libtachyon not found. Build it with:\n"
        "  cmake --preset dev-linux -DTACHYON_BUILD_SHARED_LIB=ON\n"
        "  cmake --build build/dev-linux --target tachyon_lib"
    )


class TachyonError(RuntimeError):
    pass


class Tachyon:
    """
    Thin Python wrapper around libtachyon's C API.

    All methods raise TachyonError on failure.
    """

    _ERROR_BUF_SIZE = 1024

    def __init__(self, lib_path: Optional[str] = None) -> None:
        self._lib = _find_libtachyon(lib_path)
        self._setup_signatures()
        rc = self._lib.tachyon_init()
        if rc != 0:
            raise TachyonError("tachyon_init() failed")

    # ------------------------------------------------------------------
    # Public API
    # ------------------------------------------------------------------

    @property
    def version(self) -> str:
        return self._lib.tachyon_version().decode()

    def run(self, *args: str) -> int:
        """
        Run any Tachyon CLI command.

        Example:
            engine.run("render", "--scene", "scene.cpp", "--output", "out.mp4")
        """
        argv = ["tachyon"] + list(args)
        return self._run_argv(argv)

    def render(self, scene: str, output: str, **kwargs: str) -> None:
        """Render a scene to a file."""
        cmd = ["render", "--scene", scene, "--output", output]
        for k, v in kwargs.items():
            cmd += [f"--{k.replace('_', '-')}", str(v)]
        self._run_or_raise(cmd)

    def thumb(self, scene: str, output: str,
              frame_time: float = 0.0, **kwargs: str) -> None:
        """Export a single frame as an image."""
        cmd = ["thumb", "--scene", scene, "--output", output,
               "--time", str(frame_time)]
        for k, v in kwargs.items():
            cmd += [f"--{k.replace('_', '-')}", str(v)]
        self._run_or_raise(cmd)

    def probe(self, path: str) -> str:
        """Probe a media file and return the output as a string."""
        # probe writes to stdout; capture it by redirecting inside the process
        # For simplicity, fall back to subprocess when stdout capture is needed.
        result = subprocess.run(
            [self._cli_path(), "probe", path],
            capture_output=True, text=True
        )
        if result.returncode != 0:
            raise TachyonError(f"probe failed: {result.stderr.strip()}")
        return result.stdout

    def inspect(self, scene: str) -> str:
        """Inspect a scene and return the report as a string."""
        result = subprocess.run(
            [self._cli_path(), "inspect", "--scene", scene],
            capture_output=True, text=True
        )
        if result.returncode != 0:
            raise TachyonError(f"inspect failed: {result.stderr.strip()}")
        return result.stdout

    # ------------------------------------------------------------------
    # Internals
    # ------------------------------------------------------------------

    def _setup_signatures(self) -> None:
        lib = self._lib
        lib.tachyon_version.restype = ctypes.c_char_p
        lib.tachyon_version.argtypes = []

        lib.tachyon_init.restype = ctypes.c_int
        lib.tachyon_init.argtypes = []

        lib.tachyon_run.restype = ctypes.c_int
        lib.tachyon_run.argtypes = [
            ctypes.c_int,
            ctypes.POINTER(ctypes.c_char_p),
            ctypes.c_char_p,
            ctypes.c_int,
        ]

    def _run_argv(self, argv: Sequence[str]) -> int:
        argc = len(argv)
        c_argv = (ctypes.c_char_p * argc)(*(s.encode() for s in argv))
        err_buf = ctypes.create_string_buffer(self._ERROR_BUF_SIZE)
        return self._lib.tachyon_run(argc, c_argv, err_buf, self._ERROR_BUF_SIZE)

    def _run_or_raise(self, cmd: Sequence[str]) -> None:
        rc = self._run_argv(["tachyon"] + list(cmd))
        if rc != 0:
            raise TachyonError(f"tachyon {cmd[0]} failed (exit {rc})")

    def _cli_path(self) -> str:
        # Try to find the tachyon executable next to the library
        lib_dir = Path(self._lib._name).parent  # type: ignore[attr-defined]
        for candidate in [lib_dir / "tachyon", Path("tachyon")]:
            if candidate.exists():
                return str(candidate)
        return "tachyon"


# ---------------------------------------------------------------------------
# Quick demo
# ---------------------------------------------------------------------------

if __name__ == "__main__":
    engine = Tachyon()
    print(f"Tachyon {engine.version}")

    if len(sys.argv) >= 3:
        scene, output = sys.argv[1], sys.argv[2]
        print(f"Rendering {scene} -> {output}")
        engine.render(scene, output)
        print("Done.")
    else:
        print("Usage: python tachyon.py <scene.cpp> <output.mp4>")
