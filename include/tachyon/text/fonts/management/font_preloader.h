#pragma once

#include "tachyon/text/fonts/core/font_face.h"
#include "tachyon/text/fonts/management/font_manifest.h"
#include "tachyon/text/fonts/management/font_resolver.h"

#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

namespace tachyon::text {

struct FontPreloadResult {
    std::size_t loaded{0};
    std::size_t failed{0};
    std::vector<std::string> missing_fonts;
    std::vector<std::string> failed_fonts;
    std::vector<std::string> warnings;
};

class FontPreloader {
public:
    explicit FontPreloader(FontResolver& resolver);

    FontPreloadResult preload_from_manifest(const FontManifest& manifest);
    FontPreloadResult preload_from_file(const std::filesystem::path& manifest_path);

    FontPreloadResult preload_faces(const std::vector<FontManifestEntry>& entries);
    FontPreloadResult preload_faces_for_scene(const std::vector<std::string>& font_families);

    bool validate_font_coverage(const FontManifest& manifest,
                                   const std::vector<std::uint32_t>& required_codepoints,
                                   MissingGlyphPolicy policy) const;

private:
    FontResolver& m_resolver;
    std::vector<std::unique_ptr<FontFace>> m_owned_faces;
};

} // namespace tachyon::text

