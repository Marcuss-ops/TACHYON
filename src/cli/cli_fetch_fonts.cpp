#include "tachyon/runtime/registry/engine_registry.h"
#include "tachyon/core/cli_options.h"
#include "tachyon/text/fonts/management/font_downloader.h"
#include "cli_internal.h"
#include "command_registry.h"
#include <iostream>
#include <sstream>
#include <filesystem>

namespace tachyon {

bool run_fetch_fonts_command(const CliOptions& options, std::ostream& out, std::ostream& err, runtime::EngineRegistry& /*bundle*/) {
    if (options.fonts.family.empty()) {
        err << "Error: --family is required for fetch-fonts command\n";
        return false;
    }

    text::FontFetchRequest request;
    request.family = options.fonts.family;
    request.weights = options.fonts.weights;
    request.subsets = options.fonts.subsets;
    request.dest = options.fonts.dest;
    request.overwrite = options.fonts.overwrite;

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
    out << "Destination: " << request.dest.string() << '\n';
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

CommandDescriptor make_fetch_fonts_command() {
    return {
        "fetch-fonts",
        "tachyon fetch-fonts --family <name> [--weights <w1,w2,...>] [--subsets <s1,...>] [--dest <dir>] [--overwrite]",
        nullptr,
        run_fetch_fonts_command
    };
}

} // namespace tachyon
