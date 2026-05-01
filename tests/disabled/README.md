# Disabled Tests Tracking

This directory tracks tests that are currently disabled in the Tachyon test suite.

## Purpose

Tests are disabled for various reasons:
- Pre-existing build errors
- Multiple `main()` definitions when linked together
- Features under redesign
- Temporarily disabled due to failures

## Currently Disabled Tests

### runtime_backbone
- **File**: `tests/unit/runtime/core/backbone_tests.cpp`
- **Reason**: Temporarily disabled - pre-existing failure
- **Status**: Commented out in `tests/unit/test_main.cpp` line 186
- **Last disabled**: Unknown
- **Action needed**: Investigate failure, fix, and re-enable
- **GitHub Issue**: TODO - create issue

### text_tests
- **File**: `tests/unit/text/text_tests.cpp` (not in CMakeLists.txt)
- **Reason**: Disabled - text_tests.cpp commented out of build
- **Status**: Commented out in `tests/unit/test_main.cpp` line 208
- **Action needed**: Add proper text test file and enable
- **GitHub Issue**: TODO - create issue

### motion_blur_tests
- **File**: `tests/unit/renderer3d/temporal/motion_blur_tests.cpp` (not in CMakeLists.txt)
- **Reason**: Disabled - motion_blur_tests.cpp commented out of build
- **Status**: Commented out in `tests/unit/test_main.cpp` line 214
- **Action needed**: Fix build issues and re-enable
- **GitHub Issue**: TODO - create issue

### audio_pitch_correct_tests
- **File**: `tests/unit/audio/test_audio_pitch_correct.cpp` (not in CMakeLists.txt)
- **Reason**: Disabled - test_audio_pitch_correct.cpp commented out of build
- **Status**: Commented out in `tests/unit/test_main.cpp` line 215
- **Action needed**: Fix build issues and re-enable
- **GitHub Issue**: TODO - create issue

## Policy

1. **When disabling a test**: Add an entry here with the reason and action needed
2. **When re-enabling**: Remove the entry from this file
3. **Quarantined tests**: Tests that are permanently disabled should be moved to `tests/disabled/` directory with a comment explaining why

## TODO

- [ ] Create GitHub issues for each disabled test
- [ ] Set up CI check to fail if new disabled tests are added without documentation
- [ ] Review quarterly and either fix or permanently quarantine disabled tests
