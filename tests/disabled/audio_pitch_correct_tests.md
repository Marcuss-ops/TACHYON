# audio_pitch_correct_tests

## Reason
- Disabled - test_audio_pitch_correct.cpp commented out of build

## Current failure
- Not linked in CMakeLists.txt (tracked in tests/unit/test_main.cpp line 215)

## File
- `tests/unit/audio/test_audio_pitch_correct.cpp`

## Owner
- audio

## Decision
- fix

## Re-enable command
- `.\build.ps1 -Preset dev-fast -RunTests -TestFilter audio_pitch_correct_tests`
