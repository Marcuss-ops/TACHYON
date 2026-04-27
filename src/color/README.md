# Color Management Module

This module provides advanced color management features for Tachyon, including:

## Features

- **ACES Integration**: Support for Academy Color Encoding System (ACES) color spaces (AP0, AP1).
- **OCIO Support**: OpenColorIO integration for professional color pipeline (when available).
- **Color Space Conversion**: Utilities for converting between ACES, sRGB, and working spaces.
- **Tone Mapping**: ACES tone curve application for HDR/wide gamut content.

## Files

- `include/tachyon/color/aces.h` - Header for ACES color manager and utilities.
- `src/color/aces.cpp` - Implementation of ACES color conversions and OCIO integration.

## Usage

```cpp
#include "tachyon/color/aces.h"

tachyon::color::ACESColorManager manager;
manager.initialize(tachyon::color::ACESVersion::ACES_1_1);

// Convert sRGB to ACES AP1
float rgb[3] = {1.0f, 0.5f, 0.0f};
tachyon::color::colorspace::srgb_to_aces_ap1(rgb, rgb, rgb, 1);

// Apply tone curve
manager.apply_aces_tone_curve(rgb, rgb+1, rgb+2, 1);
```

## Notes

- OCIO integration is currently a placeholder; full integration requires the OpenColorIO library.
- ACES conversions use simplified matrices; for production use, consider using the official ACES reference implementation.
