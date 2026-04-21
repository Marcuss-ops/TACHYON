# Testing

## ASAN Testing

Use the AddressSanitizer preset to catch lifetime and out-of-bounds bugs in the runtime:

```bash
.\build.ps1 -Preset asan -RunTests
```

For a faster focused pass while iterating on cache or frame execution changes:

```bash
.\build.ps1 -Preset asan -RunTests -TestFilter "frame_executor,frame_cache,scene_spec"
```

For reproducible failures, pass a fixed seed and repeat count:

```bash
.\build.ps1 -RunTests -TestSeed 12345 -TestRepeat 20
```

To discover available test groups:

```bash
.\build.ps1 -ListTests
```

If AddressSanitizer reports an error, do not merge the change until the issue is fixed and the ASAN run is clean.
