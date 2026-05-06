# Disabled Tests Tracking

This directory tracks tests that are currently disabled in the Tachyon test suite.

## Purpose

Tests are tracked here for one of four reasons:
- Pre-existing build errors
- Multiple `main()` definitions when linked together
- Features under redesign
- Temporarily disabled due to failures

## Currently Disabled Tests

### runtime_backbone
- **File**: `tests/unit/runtime/core/backbone_tests.cpp`
- **Reason**: Under review for deterministic cache-key coverage
- **Status**: Currently linked from `tests/unit/test_main.cpp`
- **Last reviewed**: 2026-05-06
- **Action needed**: Keep validating the contract or quarantine if the coverage becomes redundant
- **GitHub Issue**: #62

### text_tests
- **File**: `tests/unit/text/text_tests.cpp`
- **Reason**: Historical coverage split while text tests were moved into the content suite
- **Status**: Folded into `TachyonContentTests`
- **Action needed**: Either delete the stale entry or replace it with a focused new text contract test
- **GitHub Issue**: #63

### motion_blur_tests
- **File**: `tests/unit/renderer3d/temporal/motion_blur_tests.cpp`
- **Reason**: 3D temporal pipeline contract still needs a tighter baseline
- **Status**: Covered by the render and render-pipeline lanes
- **Action needed**: Keep the dedicated temporal contract test visible in CI
- **GitHub Issue**: #64

### audio_pitch_correct_tests
- **File**: `tests/unit/audio/test_audio_pitch_correct.cpp`
- **Reason**: Audio pitch-correction contract is still evolving
- **Status**: Included in `TachyonContentTests`
- **Action needed**: Decide whether the contract belongs in content or runtime and keep one canonical home
- **GitHub Issue**: #65

## Policy

1. **When disabling a test**: Add an entry here with the reason and action needed.
2. **When re-enabling**: Update the status or remove the entry if the debt is fully retired.
3. **Quarantined tests**: Permanently disabled tests should stay documented here with a clear explanation and owner action.

## TODO

- [x] Create GitHub issues for each disabled test
- [ ] Set up CI check to fail if new disabled tests are added without documentation
- [ ] Review quarterly and either fix or permanently quarantine disabled tests
