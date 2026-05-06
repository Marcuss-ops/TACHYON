# runtime_backbone

## Reason
- Pre-existing failure

## Current failure
- Build error / assertion / crash (tracked in tests/unit/test_main.cpp line 186)

## File
- `tests/unit/runtime/core/backbone_tests.cpp`

## Owner
- runtime

## Decision
- fix

## Re-enable command
- `.\build.ps1 -Preset dev-fast -RunTests -TestFilter runtime_backbone`
