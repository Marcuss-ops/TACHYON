# Night Agents

Use this file when sending agents to work unattended overnight.

## Rules

- Start from the current branch state. Do not reset or revert unrelated changes.
- Keep changes small and focused.
- Search existing code before adding new logic.
- Prefer extending existing modules over creating parallel systems.
- Do not create a second source of truth.
- Do not duplicate parsers, serializers, registries, caches, or render paths.
- Do not add `#include "*.cpp"` patterns.
- Keep headers lightweight.
- Keep implementation details in `.cpp` files.
- Do not introduce new build wiring unless the new file must be compiled.
- Do not touch unrelated files.

## Common Risks

- Double source of truth.
- Duplicate logic in multiple modules.
- Stale CMake or build wiring after file moves.
- Hidden dependencies through `.cpp` includes.
- Namespace drift after refactors.
- Header bloat and circular dependencies.
- Missing test coverage for touched paths.

## Before Stopping

- Verify the build still works.
- Run the targeted tests for the touched area.
- Check for duplicate symbols or duplicate definitions.
- Check that moved files are in the right target.
- Check that new public APIs are actually needed.
- Check for lingering temporary code or debug prints.

## Validation Order

1. Search the existing code.
2. Patch the smallest possible set of files.
3. Run a fast build check.
4. Fix compile or link errors.
5. Run targeted tests.
6. Run full validation if the change affects wiring or shared code.

## Good Defaults

- Reuse the existing parser or serializer if it already exists.
- Merge similar helpers into one module.
- Split large functions into smaller ones.
- Replace fragile helper includes with real translation units.
- Prefer explicit, boring code over clever shortcuts.

## Stop Conditions

- If the change introduces a second owner for the same data, stop and consolidate.
- If a file keeps growing without a clear boundary, split it.
- If a fix needs copying logic, refactor first.
- If the build passes but tests fail, the task is not done.

