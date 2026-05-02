#pragma once

#include "tachyon/core/spec/schema/contracts/shared_contracts.h"
#include <string>
#include <vector>
#include <optional>

namespace tachyon::audio {

/**
 * @brief Returns the list of available sound effect categories (folder names).
 */
std::vector<std::string> getAvailableSoundCategories();

/**
 * @brief Gets a random sound file path from the given category.
 * @param category One of: "MoneySound", "Mouse", "Photo", "Soosh", "TypeWriting"
 * @return Full path to a random .m4a file, or std::nullopt if category is empty/invalid.
 */
std::optional<std::string> getRandomSoundEffect(const std::string& category);

/**
 * @brief Gets all sound file paths from the given category.
 */
std::vector<std::string> getSoundEffectsInCategory(const std::string& category);

/**
 * @brief Creates an AudioTrackSpec with a random sound from the given category.
 * @param category Folder name (e.g. "Mouse", "Soosh")
 * @param volume Optional volume (default 1.0)
 * @return AudioTrackSpec ready to be added to a composition, or std::nullopt if failed.
 */
std::optional<spec::AudioTrackSpec> createRandomSoundTrack(
    const std::string& category, float volume = 1.0f);

} // namespace tachyon::audio
