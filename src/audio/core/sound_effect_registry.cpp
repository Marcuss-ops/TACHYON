#include "tachyon/audio/core/sound_effect_registry.h"
#include <filesystem>
#include <random>
#include <algorithm>

namespace tachyon::audio {

std::vector<std::string> getAvailableSoundCategories(const media::AssetResolver& resolver) {
    std::vector<std::string> categories;
    auto sfx_root = resolver.config().sfx_root;
    if (sfx_root.empty() || !std::filesystem::exists(sfx_root)) return categories;
    
    for (const auto& entry : std::filesystem::directory_iterator(sfx_root)) {
        if (entry.is_directory()) {
            categories.push_back(entry.path().filename().string());
        }
    }
    return categories;
}

std::vector<std::string> getSoundEffectsInCategory(const media::AssetResolver& resolver, const std::string& category) {
    std::vector<std::string> files;
    auto sfx_root = resolver.config().sfx_root;
    if (sfx_root.empty()) return files;
    
    auto dir = sfx_root / category;
    if (!std::filesystem::exists(dir)) return files;
    
    for (const auto& entry : std::filesystem::directory_iterator(dir)) {
        // We look for common audio extensions
        if (entry.is_regular_file()) {
            std::string ext = entry.path().extension().string();
            std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
            if (ext == ".m4a" || ext == ".mp3" || ext == ".wav") {
                files.push_back(entry.path().string());
            }
        }
    }
    std::sort(files.begin(), files.end()); // Ensure deterministic order for indexing
    return files;
}

std::optional<std::string> getRandomSoundEffect(const media::AssetResolver& resolver, const std::string& category, std::uint64_t seed) {
    auto files = getSoundEffectsInCategory(resolver, category);
    if (files.empty()) return std::nullopt;
    
    std::mt19937_64 rng(seed);
    std::uniform_int_distribution<size_t> dist(0, files.size() - 1);
    return files[dist(rng)];
}

std::optional<spec::AudioTrackSpec> createRandomSoundTrack(
    const media::AssetResolver& resolver, const std::string& category, std::uint64_t seed, float volume) {
    auto path = getRandomSoundEffect(resolver, category, seed);
    if (!path) return std::nullopt;
    
    spec::AudioTrackSpec spec;
    spec.source_path = *path;
    spec.volume = volume;
    return spec;
}

} // namespace tachyon::audio
