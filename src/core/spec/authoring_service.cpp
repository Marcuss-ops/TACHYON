#include "tachyon/core/spec/authoring_service.h"
#include "tachyon/tachyon_build_config.h"
#include "tachyon/core/platform/process.h"
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <sstream>
#include <vector>
#include <chrono>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#endif

namespace tachyon::core::spec {

AuthoringService& AuthoringService::get_instance() {
    static AuthoringService instance;
    return instance;
}

#ifdef _WIN32
bool AuthoringService::msvc_environment_is_ready() {
    return std::getenv("VSCMD_VER") ||
           std::getenv("VCINSTALLDIR") ||
           std::getenv("VCToolsInstallDir");
}

std::optional<std::filesystem::path> AuthoringService::find_vswhere_exe() {
    const char* program_files_x86 = std::getenv("ProgramFiles(x86)");
    if (program_files_x86) {
        const std::filesystem::path candidate =
            std::filesystem::path(program_files_x86) / "Microsoft Visual Studio" / "Installer" / "vswhere.exe";
        if (std::filesystem::exists(candidate)) {
            return candidate;
        }
    }

    const char* program_files = std::getenv("ProgramFiles");
    if (program_files) {
        const std::filesystem::path candidate =
            std::filesystem::path(program_files) / "Microsoft Visual Studio" / "Installer" / "vswhere.exe";
        if (std::filesystem::exists(candidate)) {
            return candidate;
        }
    }
    return std::nullopt;
}
std::optional<std::filesystem::path> AuthoringService::find_vcvars64_bat() {
    const std::string configured_vcvars = TACHYON_VCVARS64_BAT;
    if (!configured_vcvars.empty()) {
        const std::filesystem::path configured_path = configured_vcvars;
        if (std::filesystem::exists(configured_path)) {
            return configured_path;
        }
    }

    if (msvc_environment_is_ready()) {
        return std::nullopt;
    }

    auto search_visual_studio_roots = []() -> std::optional<std::filesystem::path> {
        const std::vector<std::filesystem::path> roots = {
            std::filesystem::path(std::getenv("ProgramFiles(x86)") ? std::getenv("ProgramFiles(x86)") : ""),
            std::filesystem::path(std::getenv("ProgramFiles") ? std::getenv("ProgramFiles") : ""),
            std::filesystem::path("C:/Program Files (x86)"),
            std::filesystem::path("C:/Program Files")
        };

        for (const auto& root : roots) {
            if (root.empty() || !std::filesystem::exists(root)) {
                continue;
            }
            const auto visual_studio_root = root / "Microsoft Visual Studio";
            if (!std::filesystem::exists(visual_studio_root)) {
                continue;
            }

            std::error_code ec;
            for (std::filesystem::recursive_directory_iterator it(
                     visual_studio_root,
                     std::filesystem::directory_options::skip_permission_denied,
                     ec);
                 it != std::filesystem::recursive_directory_iterator();
                 it.increment(ec)) {
                if (ec) {
                    continue;
                }
                if (!it->is_regular_file()) {
                    continue;
                }
                if (it->path().filename() == "vcvars64.bat") {
                    return it->path();
                }
            }
        }
        return std::nullopt;
    };

    if (const auto vswhere = find_vswhere_exe(); vswhere.has_value()) {
        using namespace tachyon::core::platform;
        ProcessSpec spec;
        spec.executable = *vswhere;
        spec.args = {"-latest", "-property", "installationPath", "-products", "*"};

        const auto proc = run_process(spec);
        if (proc.success && proc.exit_code == 0) {
            std::istringstream lines(proc.output);
            std::string installation_path;
            std::getline(lines, installation_path);
            if (!installation_path.empty()) {
                const std::filesystem::path vcvars =
                    std::filesystem::path(installation_path) / "VC" / "Auxiliary" / "Build" / "vcvars64.bat";
                if (std::filesystem::exists(vcvars)) {
                    return vcvars;
                }

                const std::filesystem::path visual_studio_root = std::filesystem::path(installation_path).parent_path();
                if (std::filesystem::exists(visual_studio_root)) {
                    std::error_code ec;
                    for (std::filesystem::recursive_directory_iterator it(
                             visual_studio_root,
                             std::filesystem::directory_options::skip_permission_denied,
                             ec);
                         it != std::filesystem::recursive_directory_iterator();
                         it.increment(ec)) {
                        if (ec) {
                            continue;
                        }
                        if (!it->is_regular_file()) {
                            continue;
                        }
                        if (it->path().filename() == "vcvars64.bat") {
                            return it->path();
                        }
                    }
                }
            }
        }
    }
    return search_visual_studio_roots();
}
#endif

bool AuthoringService::is_compiler_available() {
#ifdef _WIN32
    return msvc_environment_is_ready() || find_vcvars64_bat().has_value();
#else
    return true; // Assume system compiler available on Unix
#endif
}

std::vector<std::string> AuthoringService::get_link_libs() {
    // The JIT authoring path should stay as close as possible to a single
    // scene-authoring surface. Do not pull renderer/runtime/media/output
    // modules into the plugin boundary.
    return {
        "TachyonScene"
    };
}

std::string AuthoringService::get_compiler_command(
    const std::filesystem::path& cpp_path,
    const std::filesystem::path& dll_path) {
    
    std::stringstream ss;
#ifdef _WIN32
    if (const auto vcvars = find_vcvars64_bat(); vcvars.has_value()) {
        ss << "call \"" << vcvars->string() << "\" >nul && ";
    }
    // MSVC cl.exe command
    ss << "cl.exe /nologo /O2 /MD /EHsc /LD /std:c++20 /openmp /DNDEBUG /DTACHYON_USE_DLL /DTACHYON_SHARED /wd4190 ";
    ss << "/I\"" << TACHYON_INCLUDE_DIR << "\" ";
    ss << "\"" << cpp_path.string() << "\" ";
    ss << "/Fe:\"" << dll_path.string() << "\" ";
    ss << "/Fo:\"" << (dll_path.parent_path() / cpp_path.filename()).replace_extension(".obj").string() << "\" ";

    std::filesystem::path lib_path = TACHYON_LIB_PATH;
    if (!std::filesystem::exists(lib_path / (get_link_libs()[0] + ".lib"))) {
        if (std::filesystem::exists(lib_path / "RelWithDebInfo")) {
            lib_path /= "RelWithDebInfo";
        } else if (std::filesystem::exists(lib_path / "Release")) {
            lib_path /= "Release";
        } else if (std::filesystem::exists(lib_path / "Debug")) {
            lib_path /= "Debug";
        }
    }

    // Heuristic to detect which entry points are present
    bool has_build_scene = false;
    bool has_jit_api = false;
    {
        std::ifstream ifs(cpp_path);
        std::string content{std::istreambuf_iterator<char>(ifs), std::istreambuf_iterator<char>()};
        has_build_scene = content.find("build_scene") != std::string::npos;
        has_jit_api = content.find("tachyon_jit_build_scene") != std::string::npos;
    }

    ss << "/link ";
    if (has_build_scene) ss << "/EXPORT:build_scene ";
    if (has_jit_api) ss << "/EXPORT:tachyon_jit_build_scene /EXPORT:tachyon_jit_get_manifest ";
    const auto libs = get_link_libs();
    for (std::size_t i = 0; i < libs.size(); ++i) {
        if (i != 0) ss << ' ';
        ss << "\"" << (lib_path / (libs[i] + ".lib")).string() << "\"";
    }
#else
    // Clang/GCC command
    ss << "c++ -O3 -shared -fPIC -std=c++20 -DTACHYON_USE_DLL ";
    ss << "-I\"" << TACHYON_INCLUDE_DIR << "\" ";
    ss << "\"" << cpp_path.string() << "\" ";
    ss << "-o \"" << dll_path.string() << "\" ";
    
    const auto libs = get_link_libs();
    const std::filesystem::path lib_path = TACHYON_LIB_PATH;
    for (std::size_t i = 0; i < libs.size(); ++i) {
        if (i != 0) ss << ' ';
        std::string ext = (libs[i] == "TachyonScene") ? ".so" : ".a";
        ss << "\"" << (lib_path / ("lib" + libs[i] + ext)).string() << "\"";
    }

#endif
    return ss.str();
}

AuthoringService::CompileResult AuthoringService::compile_to_shared_lib(
    const std::filesystem::path& cpp_path,
    const std::filesystem::path& output_dir) {
    
    CompileResult result;
    std::filesystem::create_directories(output_dir);
    
    // 1. Calculate content hash
    std::string hash_str = "jit_fallback";
    {
        std::ifstream ifs(cpp_path, std::ios::binary);
        if (ifs) {
            std::string content((std::istreambuf_iterator<char>(ifs)), (std::istreambuf_iterator<char>()));
            // Simple stable hash (FNV-1a 64-bit) for content-addressing
            uint64_t hash_val = 0xcbf29ce484222325ULL;
            for (char c : content) {
                hash_val ^= static_cast<uint8_t>(c);
                hash_val *= 0x100000001b3ULL;
            }
            std::stringstream hss;
            hss << std::hex << hash_val;
            hash_str = hss.str();
        }
    }

    // 2. Generate content-addressed DLL path
    std::string dll_filename = cpp_path.stem().string() + "_" + hash_str;
#ifdef _WIN32
    dll_filename += ".dll";
#else
    dll_filename += ".so";
#endif
    std::filesystem::path dll_path = output_dir / dll_filename;

    // Optimization: reuse existing DLL if it exists to bypass redundant JIT recompiles
    if (std::filesystem::exists(dll_path)) {
        result.success = true;
        result.output_path = dll_path;
        return result;
    }

    if (!is_compiler_available()) {
        result.error = "Compiler tools not found.";
        return result;
    }

    const std::string cmd = get_compiler_command(cpp_path, dll_path);
    result.command = cmd;
    
    using namespace tachyon::core::platform;
    ProcessSpec spec;
#ifdef _WIN32
    spec.executable = "cmd.exe";
    spec.args = {"/C", cmd};
#else
    spec.executable = "sh";
    spec.args = {"-c", cmd};
#endif

    const auto start_time = std::chrono::steady_clock::now();
    const auto proc = run_process(spec);
    const auto end_time = std::chrono::steady_clock::now();
    
    result.duration_seconds = std::chrono::duration<double>(end_time - start_time).count();

    if (!proc.success || proc.exit_code != 0) {
        result.error = "Compilation failed (exit " + std::to_string(proc.exit_code) + "):\n" + proc.output + "\n" + proc.error;
        return result;
    }

#ifdef _WIN32
    const std::filesystem::path scene_dll = std::filesystem::path(TACHYON_LIB_PATH) / "RelWithDebInfo" / "TachyonScene.dll";
    if (std::filesystem::exists(scene_dll)) {
        std::error_code ec;
        std::filesystem::copy_file(scene_dll, output_dir / scene_dll.filename(), std::filesystem::copy_options::overwrite_existing, ec);
    }
#endif

    result.success = true;
    result.output_path = dll_path;
    return result;
}

} // namespace tachyon::core::spec
