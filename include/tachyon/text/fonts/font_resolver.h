#pragma once

#include "tachyon/text/fonts/font_face.h"
#include "tachyon/text/fonts/font_instance.h"

#include <cstdint>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace tachyon::text {

struct FontRequest {
    std::string family;
    std::uint32_t weight{400};
    FontStyle style{FontStyle::Normal};
    FontStretch stretch{FontStretch::Normal};
    std::uint32_t pixel_size{32};
    HintingMode hinting{HintingMode::Normal};
    RenderMode render_mode{RenderMode::Normal};
};

struct ResolvedFont {
    const FontFace* face{nullptr};
    const FontInstance* instance{nullptr};
    bool is_fallback{false};
};

class FontResolver {
public:
    FontResolver() = default;

    void clear();

    void register_face(const FontFace& face, const std::string& family,
                       std::uint32_t weight = 400,
                       FontStyle style = FontStyle::Normal);

    void set_fallback_chain(const std::string& family,
                           const std::vector<std::string>& fallbacks);

    ResolvedFont resolve(const FontRequest& request) const;

    std::vector<const FontFace*> get_faces_for_family(const std::string& family) const;

    const FontInstance* get_or_create_instance(const FontFace& face,
                                              std::uint32_t pixel_size,
                                              HintingMode hinting,
                                              RenderMode render_mode);

private:
    struct FaceKey {
        std::string family;
        std::uint32_t weight;
        FontStyle style;

        bool operator==(const FaceKey& other) const {
            return family == other.family &&
                   weight == other.weight &&
                   style == other.style;
        }
    };

    struct FaceKeyHash {
        std::size_t operator()(const FaceKey& key) const {
            std::size_t h = std::hash<std::string>{}(key.family);
            h ^= std::hash<std::uint32_t>{}(key.weight) + 0x9e3779b9 + (h << 6) + (h >> 2);
            h ^= std::hash<int>{}(static_cast<int>(key.style)) + 0x9e3779b9 + (h << 6) + (h >> 2);
            return h;
        }
    };

    std::unordered_map<FaceKey, const FontFace*, FaceKeyHash> m_faces;
    std::unordered_map<std::string, std::vector<std::string>> m_fallback_chains;

    mutable std::unordered_map<FontInstanceKey, std::unique_ptr<FontInstance>, FontInstanceKeyHash> m_instances;
};

} // namespace tachyon::text
