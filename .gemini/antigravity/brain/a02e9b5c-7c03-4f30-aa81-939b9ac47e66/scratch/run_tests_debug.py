import subprocess
import os

test_exe = r"c:\Users\pater\Pyt\Tachyon\build\tests\Debug\TachyonTests.exe"
cwd = r"c:\Users\pater\Pyt\Tachyon"

def run_cmd(cmd):
    print(f"Running: {cmd}")
    result = subprocess.run(cmd, capture_output=True, text=True, cwd=cwd)
    print(f"Exit code: {result.returncode}")
    print(f"Stdout: {result.stdout}")
    print(f"Stderr: {result.stderr}")
    return result.returncode

if __name__ == "__main__":
    run_cmd(test_exe)
