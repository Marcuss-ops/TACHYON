#!/bin/bash
# Script to automate the creation of GitHub issues for disabled tests

if ! command -v gh &> /dev/null; then
    echo "Error: GitHub CLI (gh) is not installed."
    echo "Please install it from https://cli.github.com/ and run 'gh auth login' before using this script."
    exit 1
fi

echo "Creating issues for disabled tests..."

# 1. runtime_backbone
gh issue create \
    --title "Re-enable or quarantine: runtime_backbone_tests" \
    --body "The test \`tests/unit/runtime/core/backbone_tests.cpp\` is currently disabled. 
**Action required:** Investigate the failure and decide whether to fix it in the short term, or quarantine it permanently.
See \`tests/disabled/runtime_backbone.md\` for more context." \
    --label "tech-debt,testing"

# 2. text_tests
gh issue create \
    --title "Re-enable or quarantine: text_tests" \
    --body "The test \`tests/unit/text/text_tests.cpp\` is commented out of the build.
**Action required:** The text pipeline has been recently refactored. Verify if these tests are still relevant. Fix them, or delete them if obsolete.
See \`tests/disabled/text_tests.md\` for more context." \
    --label "tech-debt,testing"

# 3. motion_blur_tests
gh issue create \
    --title "Re-enable or quarantine: motion_blur_tests" \
    --body "The test \`tests/unit/renderer3d/temporal/motion_blur_tests.cpp\` is commented out of the build.
**Action required:** Motion blur is currently out of scope for v1. Determine if this test should be permanently quarantined until Phase 5, or fixed now.
See \`tests/disabled/motion_blur_tests.md\` for more context." \
    --label "tech-debt,testing"

# 4. audio_pitch_correct_tests
gh issue create \
    --title "Re-enable or quarantine: audio_pitch_correct_tests" \
    --body "The test \`tests/unit/audio/test_audio_pitch_correct.cpp\` is commented out of the build.
**Action required:** Audio pitch correct is a stub/experimental feature. Decide whether to keep the test quarantined or delete it until the feature is actively developed.
See \`tests/disabled/audio_pitch_correct_tests.md\` for more context." \
    --label "tech-debt,testing"

echo "Done! Make sure to update tests/disabled/README.md with the new issue numbers."
