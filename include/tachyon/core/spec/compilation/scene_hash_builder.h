#pragma once
#include "tachyon/core/spec/schema/objects/scene_spec.h"
#include "tachyon/runtime/core/contracts/determinism_contract.h"
#include <cstdint>

namespace tachyon {
std::uint64_t hash_scene_spec(const SceneSpec& scene, const DeterminismContract& contract);
} // namespace tachyon
