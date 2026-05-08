#pragma once

#include "tachyon/core/spec/schema/contracts/shared_contracts.h"
#include <string>
#include <vector>
#include <optional>
#include <filesystem>

namespace tachyon::audio {

/**
 * @brief Registry for managing and resolving sound effects from the filesystem.
 * 
 * Replaces the legacy free functions with a structured, instance-based registry
 * that can be integrated with the AssetResolver and engine lifecycle.
 */
class SoundEffectRegistry {
public:
    explicit SoundEffectRegistry(std::filesystem::path root = "");

    /**
     * @brief Returns the list of available sound effect categories (subdirectories).
     */
    std::vector<std::string> getAvailableCategories() const;

    /**
     * @brief Gets all sound file paths from the given category.
     */
    std::vector<std::string> getSoundsInCategory(const std::string& category) const;

    /**
     * @brief Gets a random sound file path from the given category.
     * @return Full path to a random audio file, or std::nullopt if category is empty/invalid.
     */
    std::optional<std::string> getRandomSound(const std::string& category) const;

    /**
     * @brief Creates an AudioTrackSpec with a random sound from the given category.
     * @param category Folder name (e.g. "Mouse", "Soosh")
     * @param volume Optional volume (default 1.0)
     * @return AudioTrackSpec ready to be added to a composition, or std::nullopt if failed.
     */
    std::optional<spec::AudioTrackSpec> createRandomTrack(
        const std::string& category, float volume = 1.0f) const;

    /**
     * @brief Sets the physical root directory for sound effects.
     */
    void set_root(std::filesystem::path root);

    /**
     * @brief Returns the current root directory.
     */
    const std::filesystem::path& root() const { return m_root; }

private:
    std::filesystem::path m_root;
};

// --- Backward Compatibility Shim (using default global instance) ---

std::vector<std::string> getAvailableSoundCategories();
std::optional<std::string> getRandomSoundEffect(const std::string& category);
std::vector<std::string> getSoundEffectsInCategory(const std::string& category);
std::optional<spec::AudioTrackSpec> createRandomSoundTrack(
    const std::string& category, float volume = 1.0f);

} // namespace tachyon::audio
