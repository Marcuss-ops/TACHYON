#pragma once

#include "tachyon/core/spec/schema/objects/scene_spec.h"
#include "tachyon/runtime/core/contracts/determinism_contract.h"
#include "tachyon/runtime/cache/cache_key_builder.h"

#include <string>
#include <cstdint>

namespace tachyon {

void add_string(CacheKeyBuilder& builder, const std::string& value);

std::uint64_t hash_scene_spec(const SceneSpec& scene, const DeterminismContract& contract);

} // namespace tachyon
