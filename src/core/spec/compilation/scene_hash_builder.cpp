#include "tachyon/core/spec/compilation/scene_hash_builder.h"
#include "tachyon/runtime/cache/cache_key_builder.h"
#include <string>
#include <string_view>

namespace tachyon {

void add_string(CacheKeyBuilder& builder, const std::string& value) {
    builder.add_string(std::string_view{value});
}

} // namespace
