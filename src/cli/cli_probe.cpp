#include "tachyon/runtime/registry/runtime_registry_bundle.h"
#include "cli_internal.h"
#include "command_registry.h"
#include "tachyon/media/probe.h"
#include <iostream>

namespace tachyon {

bool run_probe_command(const CliOptions& options, std::ostream& out, std::ostream& err, runtime::RuntimeRegistryBundle& /*bundle*/) {
    if (options.probe_input.empty()) {
        err << "Error: --input is required for probe command\n";
#include "tachyon/runtime/registry/runtime_registry_bundle.h"
        return false;
    }

    auto result = media::MediaProbe::probe_file(options.probe_input);
    if (!result.ok()) {
        err << "Error probing file: " << result.error->to_diagnostic_string() << "\n";
        return false;
    }

    const auto& meta = *result.value;
    if (options.json_output) {
        out << "{\n";
        out << "  \"path\": \"" << meta.path << "\",\n";
        out << "  \"format\": \"" << meta.format << "\",\n";
        out << "  \"duration\": " << meta.duration_seconds << ",\n";
        out << "  \"size\": " << meta.size_bytes << ",\n";
        if (meta.video) {
            out << "  \"video\": {\n";
            out << "    \"codec\": \"" << meta.video->codec << "\",\n";
            out << "    \"width\": " << meta.video->width << ",\n";
            out << "    \"height\": " << meta.video->height << "\n";
            if (meta.video->fps > 0) {
                out << ",    \"fps\": " << meta.video->fps << "\n";
            } else {
                out << "\n";
            }
            out << "  }";
            if (meta.audio) out << ",";
            out << "\n";
        }
        if (meta.audio) {
            out << "  \"audio\": {\n";
            out << "    \"codec\": \"" << meta.audio->codec << "\",\n";
            out << "    \"sample_rate\": " << meta.audio->sample_rate << ",\n";
            out << "    \"channels\": " << meta.audio->channels << "\n";
            out << "  }\n";
        }
        out << "}\n";
    } else {
        out << "Media Probing Result:\n";
        out << "  Path:     " << meta.path << "\n";
        out << "  Format:   " << meta.format << "\n";
        out << "  Duration: " << meta.duration_seconds << " s\n";
        out << "  Size:     " << meta.size_bytes << " bytes\n";
        if (meta.video) {
            out << "  Video:\n";
            out << "    Codec:  " << meta.video->codec << "\n";
            out << "    Size:   " << meta.video->width << "x" << meta.video->height << "\n";
            out << "    FPS:    " << meta.video->fps << "\n";
        }
        if (meta.audio) {
            out << "  Audio:\n";
            out << "    Codec:  " << meta.audio->codec << "\n";
            out << "    Rate:   " << meta.audio->sample_rate << " Hz\n";
            out << "    Chans:  " << meta.audio->channels << "\n";
        }
    }

    return true;
}

REGISTER_COMMAND(
    "probe",
    "tachyon probe --input <file> [--json]",
    [](const CliOptions& o, std::ostream& e) {
        if (o.probe_input.empty()) {
            e << "--input is required for probe\n";
            return false;
        }
        return true;
    },
    run_probe_command
);

} // namespace tachyon
