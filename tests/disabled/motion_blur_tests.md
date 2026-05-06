# motion_blur_tests

## Reason
- Disabled - motion_blur_tests.cpp commented out of build

## Current failure
- Not linked in CMakeLists.txt (tracked in tests/unit/test_main.cpp line 214)

## File
- `tests/unit/renderer3d/temporal/motion_blur_tests.cpp`

## Owner
- renderer3d

## Decision
- fix

## Re-enable command
- `.\build.ps1 -Preset dev-fast -RunTests -TestFilter motion_blur_tests`
