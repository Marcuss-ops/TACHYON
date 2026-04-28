#pragma once

#include "tachyon/runtime/core/serialization/tbf_codec.h"
#include <cstdint>

namespace tachyon::runtime {

class TBFMigration {
public:
    static CompiledScene migrate(const CompiledScene& scene, std::uint16_t from_version);
};

} // namespace tachyon::runtime
