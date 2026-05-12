#include "tachyon/core/cli_options.h"
#include "tachyon/text/fonts/management/font_downloader.h"
#include "cli_internal.h"
#include <iostream>
#include <sstream>

namespace tachyon {

bool run_fetch_fonts_command(const CliOptions& options, std::ostream& out, std::ostream& err, TransitionRegistry& /*registry*/) {
    if (options.font_family.empty()) {
        err << "Error: --family is required for fetch-fonts command\n";
        return false;
    }

    text::FontFetchRequest request;
    request.family = options.font_family;
    request.weights = options.font_weights;
    request.subsets = options.font_subsets;
    request.dest = options.font_dest;
    request.overwrite = options.font_overwrite;

    out << "Fetching font: " << request.family << '\n';
    if (!request.weights.empty()) {
        out << "Weights: ";
        for (size_t i = 0; i < request.weights.size(); ++i) {
            if (i > 0) out << ", ";
            out << request.weights[i];
        }
        out << '\n';
    }
    out << "Subsets: ";
    for (size_t i = 0; i < request.subsets.size(); ++i) {
        if (i > 0) out << ", ";
        out << request.subsets[i];
    }
    out << '\n';
    out << "Destination: " << request.dest << '\n';
    out << "---\n";

    auto callback = [&out](const std::string& family, std::uint32_t weight, const std::string& status) {
        if (weight > 0) {
            out << "[INFO] " << family << " (weight " << weight << "): " << status << '\n';
        } else {
            out << "[INFO] " << family << ": " << status << '\n';
        }
    };

    auto result = text::FontDownloader::fetch(request, callback);

    if (!result.success) {
        err << "Error: " << result.error_message << '\n';
        return false;
    }

    out << "\nDownloaded " << result.downloaded_fonts.size() << " font(s):\n";
    for (const auto& font : result.downloaded_fonts) {
        out << "  - " << font.id << " -> " << font.src << '\n';
    }

    if (!result.downloaded_fonts.empty()) {
        std::filesystem::path manifest_path = request.dest / "font_manifest.txt";
        out << "\nTo use these fonts, register them in your C++ scene builder.\n";
        out << "Downloaded font IDs:\n";
        for (const auto& font : result.downloaded_fonts) {
            out << "  - " << font.id << " -> " << font.src << '\n';
        }
    }

    return true;
}

} // namespace tachyon
