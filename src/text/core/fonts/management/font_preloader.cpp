#include "tachyon/text/fonts/management/font_preloader.h"

#include <filesystem>
#include <string>

namespace tachyon::text {

FontPreloader::FontPreloader(FontResolver& resolver)
    : m_resolver(resolver) {}

FontPreloadResult FontPreloader::preload_from_manifest(const FontManifest& manifest) {
    FontPreloadResult result;
    result = preload_faces(manifest.fonts);

    for (const auto& fallback_config : manifest.fallbacks) {
        m_resolver.set_fallback_chain(fallback_config.primary_family,
                                      fallback_config.fallback_families);
    }

    return result;
}

FontPreloadResult FontPreloader::preload_from_file(const std::filesystem::path& manifest_path) {
    auto manifest = FontManifestParser::parse_file(manifest_path);
    if (!manifest) {
        FontPreloadResult result;
        result.failed = 1;
        result.failed_fonts.push_back(manifest_path.string());
        return result;
    }
    return preload_from_manifest(*manifest);
}

FontPreloadResult FontPreloader::preload_faces(const std::vector<FontManifestEntry>& entries) {
    FontPreloadResult result;

    for (const auto& entry : entries) {
        auto face = std::make_unique<FontFace>();
        face->set_info({
            entry.family,
            "",
            entry.weight,
            entry.style,
            entry.stretch,
            entry.src,
            entry.format,
            entry.metrics_override,
            entry.unicode_ranges,
            entry.is_fallback
        });

        if (face->load_from_file(entry.src)) {
            m_resolver.register_face(*face, entry.family, entry.weight, entry.style);
            m_owned_faces.push_back(std::move(face));
            ++result.loaded;
        } else {
            ++result.failed;
            result.failed_fonts.push_back(entry.src.string());
        }
    }

    return result;
}

FontPreloadResult FontPreloader::preload_faces_for_scene(const std::vector<std::string>& font_families) {
    FontPreloadResult result;

    for (const auto& family : font_families) {
        auto faces = m_resolver.get_faces_for_family(family);
        if (faces.empty()) {
            ++result.failed;
            result.missing_fonts.push_back(family);
        } else {
            result.loaded += faces.size();
        }
    }

    return result;
}

bool FontPreloader::validate_font_coverage(const FontManifest& manifest,
                                            const std::vector<std::uint32_t>& required_codepoints,
                                            MissingGlyphPolicy policy) const {
    for (const auto& entry : manifest.fonts) {
        for (const auto& face : m_owned_faces) {
            if (face->info().path == entry.src) {
                for (auto cp : required_codepoints) {
                    if (!face->has_glyph(cp)) {
                        if (policy == MissingGlyphPolicy::Error) {
                            return false;
                        }
                    }
                }
                break;
            }
        }
    }
    return true;
}

} // namespace tachyon::text

