# Tachyon Stabilization Plan

## Goal

Make Tachyon buildable, testable, and maintainable before adding new rendering features.

## Non-goals

- No new 3D features
- No new tracker features
- No new preview window work
- No GUI work
- No browser path
- No major renderer rewrite

## MVP validation path

1. Build TachyonCore
2. Build TachyonTests
3. Run smoke tests
4. Render one deterministic C++ Builder scene
5. Produce PNG or MP4 output
6. Verify output with golden or hash test

## Priority order

1. Fix build preset behavior
2. Gate optional module tests
3. Split CMake source lists
4. Remove GLOB_RECURSE gradually
5. Add focused render smoke scene
