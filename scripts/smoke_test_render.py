#!/usr/bin/env python3
import subprocess
import sys
import os
import json
import time

def run_command(cmd):
    print(f"[RUN] {' '.join(cmd)}")
    return subprocess.run(cmd, capture_output=True, text=True)

def main():
    # Detect platform binary path
    # Detect platform binary path
    tachyon_exe = os.environ.get("TACHYON_BIN")
    
    if not tachyon_exe or not os.path.exists(tachyon_exe):
        fallbacks = [
            "build/dev/RelWithDebInfo/tachyon.exe",
            "build/linux-relwithdebinfo/src/tachyon",
            "build/linux-make-release/src/tachyon",
            "build/src/RelWithDebInfo/tachyon.exe",
            "build/linux-ninja-debug/src/tachyon",
            "build-ninja/src/tachyon"
        ]
        for f in fallbacks:
            if os.path.exists(f):
                tachyon_exe = f
                break
    
    if not tachyon_exe or not os.path.exists(tachyon_exe):
        print(f"[FAIL] Tachyon executable not found. Build the project first.")
        sys.exit(1)

    output_dir = "out"
    if not os.path.exists(output_dir):
        os.makedirs(output_dir)

    test_output = os.path.join(output_dir, "smoke_test.mp4")
    if os.path.exists(test_output):
        os.remove(test_output)

    # Render a short 2-second scene (if we had a way to specify duration via CLI)
    # For now, we use the test_scene_10s.cpp but we might want a shorter one.
    # Assuming tachyon render --cpp works.
    render_cmd = [
        tachyon_exe, "render",
        "--cpp", "test_scene_10s.cpp",
        "--out", test_output,
        "--json"
    ]

    start_time = time.time()
    result = run_command(render_cmd)
    end_time = time.time()

    if result.returncode != 0:
        print(f"[FAIL] Render failed with code {result.returncode}")
        print(result.stderr)
        sys.exit(1)

    print(f"[OK] Render finished in {end_time - start_time:.2f} seconds")

    if not os.path.exists(test_output):
        print(f"[FAIL] Output file {test_output} not found")
        sys.exit(1)

    print(f"[OK] Output file created: {test_output} ({os.path.getsize(test_output)} bytes)")

    # Verify with ffprobe
    probe_cmd = [
        "ffprobe", "-v", "error",
        "-show_entries", "format=duration:stream=codec_type",
        "-of", "json", test_output
    ]
    probe_result = run_command(probe_cmd)
    
    if probe_result.returncode != 0:
        print(f"[FAIL] ffprobe failed to analyze output")
        sys.exit(1)

    data = json.loads(probe_result.stdout)
    streams = data.get("streams", [])
    
    has_video = any(s["codec_type"] == "video" for s in streams)
    has_audio = any(s["codec_type"] == "audio" for s in streams)

    if has_video:
        print("[OK] Video stream found")
    else:
        print("[FAIL] No video stream found")

    if has_audio:
        print("[OK] Audio stream found")
    else:
        print("[FAIL] No audio stream found (Audio muxing might have failed or scene has no audio)")

    if has_video and has_audio:
        print("[SUCCESS] Smoke test passed! Video + Audio pipeline is working.")
    elif has_video:
        print("[WARN] Only video found. Audio muxing might be disabled or failed.")
        sys.exit(2)
    else:
        print("[FAIL] Smoke test failed.")
        sys.exit(1)

if __name__ == "__main__":
    main()
