#!/usr/bin/env python3
"""
Agent 1 - Worktree Inventory

Read-only scan of the entire worktree to classify every file into categories:
- keep: Tracked files that are part of current development work
- commit_now: New untracked files that should be committed
- generated_temp: Build artifacts, logs, temp files
- suspicious: Deleted tracked files or files requiring human review

Output: 3 lists + batch commit proposal
"""

import subprocess
import re
import os
from pathlib import Path

class WorktreeInventory:
    def __init__(self, repo_root="."):
        self.repo_root = Path(repo_root).resolve()
        self.status_output = ""
        self.untracked = []
        self.modified = []
        self.deleted = []
        self.staged = []
        
        # Classification results
        self.keep = []
        self.commit_now = []
        self.generated_temp = []
        self.suspicious = []
        
        # Patterns for classification
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
        ]
        
        self.source_patterns = [
            r'src/.*\.(cpp|h|hpp|c)$',
            r'include/tachyon/.*\.(h|hpp)$',
            r'tests/.*\.(cpp|h|hpp)$',
            r'scripts/.*\.(ps1|py|sh)$',
            r'CMakeLists\.txt$',
            r'CMakePresets\.json$',
        ]
        
    def run_git_status(self):
        """Get git status in porcelain format."""
        result = subprocess.run(
            ["git", "status", "--porcelain", "-u"],
            capture_output=True,
            text=True,
            cwd=self.repo_root
        )
        self.status_output = result.stdout
        return result.stdout
    
    def parse_status(self):
        """Parse porcelain git status output."""
        for line in self.status_output.strip().split('\n'):
            if not line:
                continue
                
            status = line[:2]
            filepath = line[3:].strip()
            
            if status == '??':
                self.untracked.append(filepath)
            elif status == 'M ' or status == ' M':
                self.modified.append(filepath)
            elif status == 'D ' or status == ' D':
                self.deleted.append(filepath)
            elif status == 'A ' or status == ' A':
                self.staged.append(filepath)
    
    def is_generated_temp(self, filepath):
        """Check if file matches generated/temp patterns."""
        for pattern in self.generated_patterns:
            if re.search(pattern, filepath):
                return True
        return False
    
    def is_source_file(self, filepath):
        """Check if file is a source file that should be committed."""
        for pattern in self.source_patterns:
            if re.match(pattern, filepath):
                return True
        return False
    
    def classify_files(self):
        """Classify all files into categories."""
        
        # Classify untracked files (??)
        for f in self.untracked:
            if self.is_generated_temp(f):
                self.generated_temp.append(f)
            elif self.is_source_file(f) or f.endswith('.md') or f.endswith('.json'):
                self.commit_now.append(f)
            else:
                # Check if it's a directory
                full_path = self.repo_root / f
                if full_path.is_dir():
                    if f.startswith('include/tachyon/') or f.startswith('src/') or f.startswith('tests/'):
                        self.commit_now.append(f)
                    else:
                        self.generated_temp.append(f)
                else:
                    self.suspicious.append(f)
        
        # Classify modified files (M)
        for f in self.modified:
            if self.is_source_file(f) or f in ['task.md', 'CMakeLists.txt', 'CMakePresets.json']:
                self.keep.append(f)  # Part of current work, already tracked
            else:
                self.suspicious.append(f)
        
        # Classify deleted files (D) - these are suspicious
        for f in self.deleted:
            self.suspicious.append(f)
    
    def generate_commit_proposal(self):
        """Generate batch commit proposal for commit_now files."""
        if not self.commit_now:
            return "No files to commit.\n"
        
        # Group by category for commit message
        new_dirs = [f for f in self.commit_now if f.endswith('/') or (self.repo_root / f).is_dir()]
        new_files = [f for f in self.commit_now if f not in new_dirs]
        
        proposal = "\n=== BATCH COMMIT PROPOSAL ===\n\n"
        proposal += "Commit Message:\n"
        proposal += "\"feat: add new subsystems and update existing modules\"\n\n"
        proposal += "Files to commit:\n"
        
        if new_dirs:
            proposal += "\n# New directories:\n"
            for d in sorted(new_dirs):
                proposal += f"  {d}\n"
        
        if new_files:
            proposal += "\n# New files:\n"
            for f in sorted(new_files):
                proposal += f"  {f}\n"
        
        return proposal
    
    def print_report(self):
        """Print the full inventory report."""
        print("=" * 60)
        print("AGENT 1: WORKTREE INVENTORY")
        print("=" * 60)
        
        print("\n1. KEEP (tracked, modified files - part of current work):")
        print("-" * 60)
        if self.keep:
            for f in sorted(self.keep):
                print(f"  {f}")
        else:
            print("  (none)")
        
        print("\n2. COMMIT NOW (untracked new files ready to commit):")
        print("-" * 60)
        if self.commit_now:
            for f in sorted(self.commit_now):
                print(f"  {f}")
        else:
            print("  (none)")
        
        print("\n3. GENERATED/TEMP (should be .gitignore'd or deleted):")
        print("-" * 60)
        if self.generated_temp:
            for f in sorted(self.generated_temp):
                print(f"  {f}")
        else:
            print("  (none)")
        
        print("\n4. SUSPICIOUS (require human review):")
        print("-" * 60)
        if self.suspicious:
            for f in sorted(self.suspicious):
                print(f"  {f}")
        else:
            print("  (none)")
        
        print("\n" + "=" * 60)
        print(self.generate_commit_proposal())
        print("=" * 60)
    
    def run(self):
        """Execute the full inventory."""
        print("Running git status...")
        self.run_git_status()
        
        print("Parsing status...")
        self.parse_status()
        
        print("Classifying files...")
        self.classify_files()
        
        print("Generating report...\n")
        self.print_report()


if __name__ == "__main__":
    agent = WorktreeInventory()
    agent.run()