#include "sound_effect_registry.h"

#include <algorithm>
#include <filesystem>
#include <random>

namespace tachyon::audio {

namespace {

std::filesystem::path default_sound_effect_root() {
    return std::filesystem::current_path() / "assets" / "audio";
}

std::vector<std::string> collect_audio_files(const std::filesystem::path& root) {
    std::vector<std::string> files;
    if (root.empty() || !std::filesystem::exists(root)) {
        return files;
    }

    for (const auto& entry : std::filesystem::directory_iterator(root)) {
        if (!entry.is_regular_file()) {
            continue;
        }

        std::string ext = entry.path().extension().string();
        std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
        if (ext == ".m4a" || ext == ".mp3" || ext == ".wav") {
            files.push_back(entry.path().string());
        }
    }

    std::sort(files.begin(), files.end());
    return files;
}

template <typename T>
std::optional<T> pick_random_entry(const std::vector<T>& values) {
    if (values.empty()) {
        return std::nullopt;
    }

    static thread_local std::mt19937 rng{std::random_device{}()};
    std::uniform_int_distribution<std::size_t> dist(0, values.size() - 1);
    return values[dist(rng)];
}

} // namespace
SoundEffectRegistry::SoundEffectRegistry(std::filesystem::path root)
    : m_root(root.empty() ? default_sound_effect_root() : std::move(root)) {}

void SoundEffectRegistry::set_root(std::filesystem::path root) {
    m_root = std::move(root);
}

std::vector<std::string> SoundEffectRegistry::getAvailableCategories() const {
    std::vector<std::string> categories;
    if (m_root.empty() || !std::filesystem::exists(m_root)) {
        return categories;
    }

    for (const auto& entry : std::filesystem::directory_iterator(m_root)) {
        if (entry.is_directory()) {
            categories.push_back(entry.path().filename().string());
        }
    }

    std::sort(categories.begin(), categories.end());
    return categories;
}

std::vector<std::string> SoundEffectRegistry::getSoundsInCategory(const std::string& category) const {
    return collect_audio_files(m_root / category);
}

std::optional<std::string> SoundEffectRegistry::getRandomSound(const std::string& category) const {
    return pick_random_entry(getSoundsInCategory(category));
}

std::optional<spec::AudioTrackSpec> SoundEffectRegistry::createRandomTrack(const std::string& category, float volume) const {
    const auto path = getRandomSound(category);
    if (!path.has_value()) {
        return std::nullopt;
    }

    spec::AudioTrackSpec spec;
    spec.source_path = *path;
    spec.volume = volume;
    return spec;
}

} // namespace tachyon::audio
