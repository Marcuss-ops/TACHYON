#!/usr/bin/env python3
import subprocess
import sys
import os

def check_command(cmd, args=["--version"]):
    try:
        result = subprocess.run([cmd] + args, capture_output=True, text=True)
        if result.returncode == 0:
            version = result.stdout.split('\n')[0]
            print(f"[OK] {cmd} found: {version}")
            return True
        else:
            print(f"[WARN] {cmd} found but returned error code {result.returncode}")
            return False
    except FileNotFoundError:
        print(f"[FAIL] {cmd} NOT found in PATH")
        return False

def main():
    print("--- Tachyon Environment Doctor ---")
    
    all_ok = True
    
    # Build tools
    all_ok &= check_command("cmake")
    if os.name == "nt":
        all_ok &= check_command("cl.exe", args=[]) # MSVC
    else:
        all_ok &= (check_command("clang++") or check_command("g++"))
        all_ok &= check_command("ninja")

    # Runtime dependencies
    all_ok &= check_command("ffmpeg", args=["-version"])
    all_ok &= check_command("ffprobe", args=["-version"])

    print("-----------------------------------")
    if all_ok:
        print("[SUCCESS] Environment looks good for Tachyon development and rendering!")
    else:
        print("[FAIL] Some dependencies are missing. Please install them and ensure they are in your PATH.")
        sys.exit(1)

if __name__ == "__main__":
    main()
