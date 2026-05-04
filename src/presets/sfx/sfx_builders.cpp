#include "tachyon/presets/sfx/sfx_builders.h"
#include <random>
#include <filesystem>
#include <map>
#include <vector>
#include <algorithm>

namespace tachyon::presets {

namespace spec = tachyon::spec;
using spec::AudioTrackSpec;

namespace {

std::filesystem::path resolve_sfx_path(SfxCategory category, int variant, uint64_t seed) {
    static const std::map<SfxCategory, std::string> category_folders = {
        {SfxCategory::TypeWriting, "TypeWriting"},
        {SfxCategory::Mouse, "Mouse"},
        {SfxCategory::Photo, "Photo"},
        {SfxCategory::Soosh, "Soosh"},
        {SfxCategory::MoneySound, "MoneySound"},
    };
    auto it = category_folders.find(category);
    if (it == category_folders.end()) {
        return {};
    }
    
    std::filesystem::path root;
#ifdef TACHYON_SFX_ROOT
    root = TACHYON_SFX_ROOT;
#else
    // Fallback for runtime or environments where the macro is missing
    if (const char* env_root = std::getenv("TACHYON_SFX_ROOT")) {
        root = env_root;
    } else {
        root = "audio/SoundEffect"; // Default relative path
    }
#endif

    std::filesystem::path base = root / it->second;
    
    if (variant == -1) {
        std::vector<std::filesystem::path> m4a_files;
        try {
            if (std::filesystem::exists(base)) {
                for (const auto& entry : std::filesystem::directory_iterator(base)) {
                    if (entry.path().extension() == ".m4a") {
                        m4a_files.push_back(entry.path());
                    }
                }
            }
        } catch (...) {
            return {};
        }
        
        if (m4a_files.empty()) {
            return {};
        }
        
        // Sort files to ensure deterministic index selection across different platforms
        std::sort(m4a_files.begin(), m4a_files.end());
        
        // Use seed for deterministic random selection
        std::mt19937 gen(static_cast<uint32_t>(seed));
        std::uniform_int_distribution<> dist(0, static_cast<int>(m4a_files.size()) - 1);
        return m4a_files[dist(gen)];
    } else {
        return base / (std::to_string(variant) + ".m4a");
    }
}

} // anonymous namespace

AudioTrackSpec build_sfx(const SfxParams& p) {
    AudioTrackSpec spec;
    spec.source_path = resolve_sfx_path(p.category, p.variant, p.seed).string();
    spec.in_point_seconds = static_cast<double>(p.in_point);
    spec.out_point_seconds = static_cast<double>(p.out_point);
    spec.volume = p.volume;
    return spec;
}

AudioTrackSpec build_sfx(SfxCategory cat, double trigger_time, float volume) {
    SfxParams p;
    p.category = cat;
    p.in_point = trigger_time;
    p.volume = volume;
    p.seed = 0; // Default seed for shortcut API
    return build_sfx(p);
}

} // namespace tachyon::presets
