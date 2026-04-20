#pragma once

// Minimal FFmpeg configuration shim for the bundled headers in this workspace.
// The installed package ships the public headers but not the generated avconfig.h.

#define AV_HAVE_BIGENDIAN 0
#define AV_HAVE_FAST_UNALIGNED 1
