#pragma once

#include <stdexcept>
#include <string>

/**
 * @def TACHYON_STUB(msg)
 * @brief Macro to mark unfinished code paths that should not be reached in production.
 * Throws a std::runtime_error when reached.
 */
#define TACHYON_STUB(msg) \
    throw std::runtime_error(std::string("Tachyon stub reached: ") + msg + " at " + __FILE__ + ":" + std::to_string(__LINE__))

/**
 * @def TACHYON_TODO(msg)
 * @brief Macro to mark code that needs implementation but is not critical for immediate execution.
 * Currently just a compiler hint/comment, could be extended to log.
 */
#define TACHYON_TODO(msg) // TODO: msg
