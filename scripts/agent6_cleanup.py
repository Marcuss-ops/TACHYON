#!/usr/bin/env python3
"""
Agent 6 - Pulizia artefatti (Artifact Cleanup)

Read-only scan first: identifies generated/temp files that are SAFE to delete.
Only deletes files that Agent 1 marked as "generated/temp" with high confidence.
Does NOT delete anything ambiguous.

Typical candidates from Agent 1:
- build_output.txt
- tmp_audio_mixer_header.txt
- docs/rule_violations_* reports

Safety rules:
- Only delete files explicitly marked as generated/temp by Agent 1
- Never delete files in source directories (src/, include/, tests/)
- Never delete tracked files (only untracked or ignored files)
- Require explicit --confirm flag to actually delete
- Default mode is DRY-RUN (read-only)
"""

import subprocess
import re
import os
import sys
from pathlib import Path
import argparse

class ArtifactCleanup:
    def __init__(self, repo_root=".", confirm=False):
        self.repo_root = Path(repo_root).resolve()
        self.confirm = confirm
        self.dry_run = not confirm
        
        # Files that Agent 1 identified as generated/temp
        # These are considered SAFE to delete
        self.safe_to_delete = [
            "build_output.txt",
            "tmp_audio_mixer_header.txt",
        ]
        
        # Patterns for generated/temp files (from Agent 1)
        self.generated_patterns = [
            r'build/',
            r'out/',
            r'\.git/',
            r'\.vs/',
            r'\.idea/',
            r'__pycache__/',
            r'\.pyc$',
            r'\.tmp$',
            r'\.log$',
            r'build_output\.txt$',
            r'tmp_.*\.txt$',
            r'-p/$',
            r'scratch/',
            r'\.vcxproj\.user$',
            r'\.suo$',
            r'\.VC\.db$',
            r'\.VC\.VC\.opendb$',
            r'logs/',
            r'docs/rule_violations_.*\.md$',
            r'docs/current_rule_violations\.md$',
        ]
        
        # Files found in worktree that match patterns
        self.candidates = []
        self.ambiguous = []
        self.deleted = []
        
    def run_git_status(self):
        """Get git status to find untracked files."""
        result = subprocess.run(
            ["git", "status", "--porcelain", "-u"],
            capture_output=True,
            text=True,
            cwd=self.repo_root
        )
        return result.stdout
    
    def is_safe_to_delete(self, filepath):
        """Check if file is safe to delete (matches generated patterns and not ambiguous)."""
        # Check explicit safe list
        if filepath in self.safe_to_delete:
            return True
        
        # Check patterns
        for pattern in self.generated_patterns:
            if re.search(pattern, filepath):
                return True
        
        return False
    
    def is_ambiguous(self, filepath):
        """Check if file might be ambiguous (should not be deleted)."""
        # Never delete source files
        source_prefixes = [
            'src/', 'include/', 'tests/', 'scripts/',
            'docs/NextImplementation/', 'docs/ProfessionalFeatureImplementation.md'
        ]
        for prefix in source_prefixes:
            if filepath.startswith(prefix):
                return True
        
        # Never delete tracked files
        result = subprocess.run(
            ["git", "ls-files", "--error-unmatch", filepath],
            capture_output=True,
            text=True,
            cwd=self.repo_root
        )
        if result.returncode == 0:
            return True  # File is tracked
        
        return False
    
    def scan_worktree(self):
        """Scan worktree for artifact candidates."""
        status_output = self.run_git_status()
        
        for line in status_output.strip().split('\n'):
            if not line:
                continue
            
            status = line[:2]
            filepath = line[3:].strip()
            
            # Only consider untracked files (??) for deletion
            if status != '??':
                continue
            
            # Check if it's a file (not directory)
            full_path = self.repo_root / filepath
            if not full_path.exists() or not full_path.is_file():
                continue
            
            # Check if safe to delete
            if self.is_safe_to_delete(filepath):
                if self.is_ambiguous(filepath):
                    self.ambiguous.append(filepath)
                else:
                    self.candidates.append(filepath)
    
    def delete_files(self):
        """Delete the candidate files (only if not dry-run)."""
        if self.dry_run:
            print("DRY-RUN MODE: No files will be deleted.")
            print("Use --confirm to actually delete files.\n")
            return
        
        print("CONFIRMED MODE: Deleting files...\n")
        for f in self.candidates:
            full_path = self.repo_root / f
            try:
                full_path.unlink()
                self.deleted.append(f)
                print(f"  DELETED: {f}")
            except Exception as e:
                print(f"  ERROR deleting {f}: {e}")
    
    def print_report(self):
        """Print the cleanup report."""
        print("=" * 60)
        print("AGENT 6: PULIZIA ARTEFATTI (Artifact Cleanup)")
        print("=" * 60)
        
        print("\nScan Mode:", "DRY-RUN (read-only)" if self.dry_run else "CONFIRMED (will delete)")
        print("-" * 60)
        
        print("\n1. SAFE TO DELETE (generated/temp files):")
        print("-" * 60)
        if self.candidates:
            for f in sorted(self.candidates):
                print(f"  [OK] {f}")
        else:
            print("  (none found)")
        
        print("\n2. AMBIGUOUS (will NOT delete):")
        print("-" * 60)
        if self.ambiguous:
            for f in sorted(self.ambiguous):
                print(f"  [SKIP] {f}")
        else:
            print("  (none)")
        
        print("\n3. DELETION RESULT:")
        print("-" * 60)
        if self.dry_run:
            print("  DRY-RUN: No files deleted.")
            print("  Run with --confirm to delete the above files.")
        elif self.deleted:
            print(f"  Deleted {len(self.deleted)} file(s):")
            for f in self.deleted:
                print(f"    - {f}")
        else:
            print("  No files were deleted.")
        
        print("\n" + "=" * 60)
        print("SAFETY NOTES:")
        print("- Only files marked as 'generated/temp' by Agent 1 are candidates")
        print("- No source files (src/, include/, tests/) will be deleted")
        print("- No tracked files will be deleted")
        print("- Ambiguous files are always skipped")
        print("=" * 60)
    
    def run(self):
        """Execute the cleanup agent."""
        print("Scanning worktree for artifacts...")
        self.scan_worktree()
        
        print("Generating report...\n")
        self.print_report()
        
        if not self.dry_run:
            self.delete_files()
        else:
            self.print_report()


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description='Agent 6 - Artifact Cleanup (Read-only by default)')
    parser.add_argument('--confirm', action='store_true',
                        help='Actually delete files (default is dry-run)')
    
    args = parser.parse_args()
    
    agent = ArtifactCleanup(confirm=args.confirm)
    agent.run()