#pragma once

namespace tachyon::backends {

/**
 * @brief Initializes all compiled-in backends.
 * 
 * This should be called once at engine startup.
 */
void initialize_all_backends();

} // namespace tachyon::backends
