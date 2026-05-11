# Third Party Inventory

This directory contains vendored or mirrored dependencies that are checked in because they are needed for local builds, CI, or offline fallback support.

## Policy

- Record the upstream source for each vendored component.
- Record the license and version used.
- Prefer a clean update path over patching in place.
- Do not mix ad-hoc debug assets with vendored library code.

## Current Contents

- `stb/` - Single-header image helpers used by rendering and tests.
- `libavutil/` - FFmpeg-related headers mirrored for media integration.
- `embree-3.13.5.x86_64.linux/` - Intel Embree ray tracing library (Linux x64).
- `oneapi-tbb-2021.5.0/` - Intel oneAPI Threading Building Blocks (TBB).

## Update Checklist

1. Verify the upstream version and license.
2. Confirm the build still works with the updated dependency.
3. Update any notes in the owning feature docs if the dependency affects behavior.
