import os
import sys
import re

# Architectural rules: forbidden includes for each layer
# Key: directory to scan
# Value: list of forbidden include patterns
FORBIDDEN = {
    "src/core": [
        "tachyon/ops/",
        "tachyon/backends/",
        "tachyon/media/",
    ],
    "include/tachyon/core": [
        "tachyon/ops/",
        "tachyon/backends/",
        "tachyon/media/",
    ],
    "src/runtime": [
        "tachyon/ops/",
    ],
    "operations": [
        "tachyon/media/resolution/",
        "tachyon/media/import/",
        "tachyon/presets/sfx/",
    ],
}

def check_file(file_path, forbidden_patterns):
    violations = []
    with open(file_path, 'r', encoding='utf-8', errors='ignore') as f:
        for line_num, line in enumerate(f, 1):
            if line.strip().startswith('#include'):
                for pattern in forbidden_patterns:
                    if pattern in line:
                        violations.append((line_num, line.strip(), pattern))
    return violations

def main():
    root_dir = os.path.abspath(os.path.join(os.path.dirname(__file__), ".."))
    total_violations = 0

    for scan_dir, forbidden_patterns in FORBIDDEN.items():
        full_scan_dir = os.path.join(root_dir, scan_dir)
        if not os.path.exists(full_scan_dir):
            continue

        print(f"Scanning {scan_dir} for forbidden includes...")
        for root, _, files in os.walk(full_scan_dir):
            for file in files:
                if file.endswith(('.h', '.cpp', '.hpp', '.c')):
                    file_path = os.path.join(root, file)
                    rel_path = os.path.relpath(file_path, root_dir)
                    violations = check_file(file_path, forbidden_patterns)
                    if violations:
                        for line_num, content, pattern in violations:
                            print(f"  VIOLATION in {rel_path}:{line_num}")
                            print(f"    Code: {content}")
                            print(f"    Reason: Include of '{pattern}' is forbidden in {scan_dir}")
                            total_violations += 1

    if total_violations > 0:
        print(f"\nTotal violations found: {total_violations}")
        sys.exit(1)
    else:
        print("\nNo architectural include violations found.")
        sys.exit(0)

if __name__ == "__main__":
    main()
