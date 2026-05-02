#include "tachyon/audio/core/sound_effect_registry.h"
#include "tachyon/core/spec/schema/contracts/shared_contracts.h"
#include <filesystem>
#include <random>
#include <algorithm>

namespace tachyon::audio {

namespace {

const std::filesystem::path kSoundEffectBaseDir = TACHYON_SFX_ROOT;

}

std::vector<std::string> getAvailableSoundCategories() {
    std::vector<std::string> categories;
    if (!std::filesystem::exists(kSoundEffectBaseDir)) return categories;
    for (const auto& entry : std::filesystem::directory_iterator(kSoundEffectBaseDir)) {
        if (entry.is_directory()) {
            categories.push_back(entry.path().filename().string());
        }
    }
    return categories;
}

std::vector<std::string> getSoundEffectsInCategory(const std::string& category) {
    std::vector<std::string> files;
    auto dir = kSoundEffectBaseDir / category;
    if (!std::filesystem::exists(dir)) return files;
    for (const auto& entry : std::filesystem::directory_iterator(dir)) {
        if (entry.is_regular_file() && entry.path().extension() == ".m4a") {
            files.push_back(entry.path().string());
        }
    }
    return files;
}

std::optional<std::string> getRandomSoundEffect(const std::string& category) {
    auto files = getSoundEffectsInCategory(category);
    if (files.empty()) return std::nullopt;
    static std::mt19937 rng(std::random_device{}());
    std::uniform_int_distribution<size_t> dist(0, files.size() - 1);
    return files[dist(rng)];
}

std::optional<spec::AudioTrackSpec> createRandomSoundTrack(
    const std::string& category, float volume) {
    auto path = getRandomSoundEffect(category);
    if (!path) return std::nullopt;
    spec::AudioTrackSpec spec;
    spec.source_path = *path;
    spec.volume = volume;
    return spec;
}

} // namespace tachyon::audio
