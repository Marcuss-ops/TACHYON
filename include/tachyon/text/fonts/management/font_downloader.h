#pragma once

#include "tachyon/text/fonts/management/font_manifest.h"

#include <cstdint>
#include <filesystem>
#include <functional>
#include <optional>
#include <string>
#include <vector>

namespace tachyon::text {

struct FontFetchRequest {
    std::string family;
    std::vector<std::uint32_t> weights = {400};
    std::vector<std::string> subsets = {"latin"};
    std::filesystem::path dest = "assets/fonts";
    FontStyle style = FontStyle::Normal;
    bool overwrite = false;
};

struct FontFetchResult {
    bool success = false;
    std::string error_message;
    std::vector<FontManifestEntry> downloaded_fonts;
    std::filesystem::path manifest_path;
};

class FontDownloader {
public:
    using ProgressCallback = std::function<void(const std::string& family, std::uint32_t weight, const std::string& status)>;

    static FontFetchResult fetch(const FontFetchRequest& request, ProgressCallback callback = nullptr);

    static std::optional<std::string> fetch_css(const std::string& family, const std::vector<std::uint32_t>& weights, const std::vector<std::string>& subsets);
    static std::vector<std::string> parse_font_urls(const std::string& css);
    static std::string generate_font_id(const std::string& family, std::uint32_t weight);
    static std::string sanitize_filename(const std::string& input);

private:
    static std::string build_google_fonts_url(const std::string& family, const std::vector<std::uint32_t>& weights, const std::vector<std::string>& subsets);
    static std::optional<std::vector<std::byte>> download_file(const std::string& url);
    static std::string url_encode(const std::string& str);
};

} // namespace tachyon::text
