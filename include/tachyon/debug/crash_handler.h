#pragma once

namespace tachyon::debug {

/**
 * Initializes the global crash handler.
 * On Windows: Sets up unhandled exception filter for minidumps.
 * On Linux: Sets up signal handlers for backtraces.
 */
void setup_crash_handler();

} // namespace tachyon::debug
