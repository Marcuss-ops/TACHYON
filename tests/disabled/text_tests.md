# text_tests

## Reason
- Disabled - text_tests.cpp commented out of build

## Current failure
- Not linked in CMakeLists.txt (tracked in tests/unit/test_main.cpp line 208)

## File
- `tests/unit/text/text_tests.cpp`

## Owner
- text

## Decision
- fix

## Re-enable command
- `.\build.ps1 -Preset dev-fast -RunTests -TestFilter text_tests`
