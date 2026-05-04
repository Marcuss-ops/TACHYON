#pragma once

#include "tachyon/core/spec/schema/contracts/shared_contracts.h"
#include "tachyon/media/management/asset_resolver.h"
#include <string>
#include <vector>
#include <optional>
#include <random>

namespace tachyon::audio {

/**
 * @brief Returns the list of available sound effect categories (folder names).
 */
std::vector<std::string> getAvailableSoundCategories(const media::AssetResolver& resolver);

/**
 * @brief Gets a deterministic random sound file path from the given category.
 */
std::optional<std::string> getRandomSoundEffect(const media::AssetResolver& resolver, const std::string& category, std::uint64_t seed);

/**
 * @brief Gets all sound file paths from the given category.
 */
std::vector<std::string> getSoundEffectsInCategory(const media::AssetResolver& resolver, const std::string& category);

/**
 * @brief Creates an AudioTrackSpec with a random sound from the given category.
 */
std::optional<spec::AudioTrackSpec> createRandomSoundTrack(
    const media::AssetResolver& resolver, const std::string& category, std::uint64_t seed, float volume = 1.0f);

} // namespace tachyon::audio
