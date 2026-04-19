#include "tachyon/cli.h"

#include "tachyon/core.h"
#include "tachyon/render_job.h"
#include "tachyon/scene_spec.h"

#include <filesystem>
#include <iostream>
#include <string>
#include <vector>

namespace tachyon {
namespace {

void print_diagnostics(const DiagnosticBag& diagnostics, std::ostream& out) {
    for (const auto& diagnostic : diagnostics.diagnostics) {
        out << diagnostic.code << ": " << diagnostic.message;
        if (!diagnostic.path.empty()) {
            out << " (" << diagnostic.path << ")";
        }
        out << '\n';
    }
}

std::string require_argument(const std::vector<std::string>& args, std::size_t& index) {
    if (index + 1 >= args.size()) {
        return {};
    }
    ++index;
    return args[index];
}

void print_help(std::ostream& out) {
    out << "TACHYON " << version_string() << '\n';
    out << "Usage:\n";
    out << "  tachyon version\n";
    out << "  tachyon validate --scene <file>\n";
    out << "  tachyon render --scene <file> --job <file> [--out <file>]\n";
}

} // namespace

int run_cli(int argc, char** argv) {
    const std::vector<std::string> args(argv + 1, argv + argc);
    if (args.empty()) {
        print_help(std::cout);
        return 1;
    }

    const std::string command = args[0];
    if (command == "version") {
        std::cout << version_string() << '\n';
        return 0;
    }

    std::filesystem::path scene_path;
    std::filesystem::path job_path;
    std::filesystem::path output_override;

    for (std::size_t index = 1; index < args.size(); ++index) {
        const std::string& arg = args[index];
        if (arg == "--scene") {
            const std::string value = require_argument(args, index);
            if (value.empty()) {
                std::cerr << "missing value for --scene\n";
                return 1;
            }
            scene_path = value;
            continue;
        }
        if (arg == "--job") {
            const std::string value = require_argument(args, index);
            if (value.empty()) {
                std::cerr << "missing value for --job\n";
                return 1;
            }
            job_path = value;
            continue;
        }
        if (arg == "--out") {
            const std::string value = require_argument(args, index);
            if (value.empty()) {
                std::cerr << "missing value for --out\n";
                return 1;
            }
            output_override = value;
            continue;
        }
    }

    if (command == "validate") {
        if (scene_path.empty()) {
            std::cerr << "--scene is required\n";
            return 1;
        }
        const auto parsed = parse_scene_spec_file(scene_path);
        if (!parsed.value.has_value()) {
            print_diagnostics(parsed.diagnostics, std::cerr);
            return 2;
        }
        const auto validation = validate_scene_spec(*parsed.value);
        if (!validation.ok()) {
            print_diagnostics(validation.diagnostics, std::cerr);
            return 2;
        }
        std::cout << "scene spec valid\n";
        return 0;
    }

    if (command == "render") {
        if (scene_path.empty() || job_path.empty()) {
            std::cerr << "--scene and --job are required\n";
            return 1;
        }

        const auto scene = parse_scene_spec_file(scene_path);
        if (!scene.value.has_value()) {
            print_diagnostics(scene.diagnostics, std::cerr);
            return 2;
        }
        const auto scene_validation = validate_scene_spec(*scene.value);
        if (!scene_validation.ok()) {
            print_diagnostics(scene_validation.diagnostics, std::cerr);
            return 2;
        }

        const auto job = parse_render_job_file(job_path);
        if (!job.value.has_value()) {
            print_diagnostics(job.diagnostics, std::cerr);
            return 2;
        }

        RenderJob resolved_job = *job.value;
        if (!output_override.empty()) {
            resolved_job.output.destination.path = output_override.string();
        }

        const auto job_validation = validate_render_job(resolved_job);
        if (!job_validation.ok()) {
            print_diagnostics(job_validation.diagnostics, std::cerr);
            return 2;
        }

        std::cout << "render plan valid\n";
        std::cout << "scene: " << scene_path.string() << '\n';
        std::cout << "job: " << job_path.string() << '\n';
        std::cout << "output: " << resolved_job.output.destination.path << '\n';
        std::cout << "note: pixel rendering is not wired yet; this command validates the contract slice\n";
        return 0;
    }

    print_help(std::cerr);
    return 1;
}

} // namespace tachyon
