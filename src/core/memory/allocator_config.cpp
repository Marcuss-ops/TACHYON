#include "tachyon/core/memory/allocator_config.h"

namespace tachyon::core::memory {

const char *active_allocator_name() {
#if defined(TACHYON_ENABLE_MIMALLOC)
  return "mimalloc";
#else
  return "system";
#endif
}

} // namespace tachyon::core::memory
