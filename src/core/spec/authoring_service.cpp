#include <cstdlib>
#include <fstream>
#include <iostream>
#include <sstream>
#include <vector>
#include <chrono>
#include <system_error>
#include <stdexcept>

#include "tachyon/core/spec/authoring_service.h"
#include "tachyon/tachyon_build_config.h"
#include "tachyon/core/platform/process.h"
#include "tachyon/jit/tachyon_jit_api.h"

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
        "TachyonScene",
        "TachyonCore"
    };
}



namespace {
struct JitConfig {
    std::string generator;
    bool multi_config = false;
    std::string active_config;
    std::string compiler;
    std::string cxx_standard;
    std::vector<std::string> include_dirs;
    std::vector<std::string> link_libraries;
    std::vector<std::string> compile_definitions;
    std::string vcvars64_bat;
    std::string platform;
    std::string module_suffix;
};

std::optional<JitConfig> load_jit_config() {
    // Mandate: Zero JSON. Fallback to macros or TOML.
    // For now, return nullopt to force the robust fallback path which uses build macros.
    return std::nullopt;
}
}

std::string AuthoringService::get_compiler_command(
    const std::filesystem::path& cpp_path,
    const std::filesystem::path& dll_path) {
    
    auto cfg_opt = load_jit_config();
    std::stringstream ss;

    if (cfg_opt) {
        const auto& cfg = *cfg_opt;
#ifdef _WIN32
        if (!cfg.vcvars64_bat.empty() && std::filesystem::exists(cfg.vcvars64_bat)) {
            ss << "call \"" << cfg.vcvars64_bat << "\" >nul && ";
        } else if (const auto vcvars = find_vcvars64_bat(); vcvars.has_value()) {
            ss << "call \"" << vcvars->string() << "\" >nul && ";
        }
        ss << "cl.exe /nologo /O2 /MD /EHsc /LD /std:c++" << cfg.cxx_standard << " /openmp /DNDEBUG /DTACHYON_JIT_EXPORTS /wd4190 ";
        for (const auto& d : cfg.compile_definitions) ss << "/D" << d << " ";
        for (const auto& i : cfg.include_dirs) ss << "/I\"" << i << "\" ";
        ss << "\"" << cpp_path.string() << "\" ";
        ss << "/Fe:\"" << dll_path.string() << "\" ";
        auto obj_path = dll_path;
        obj_path.replace_extension(".obj");
        ss << "/Fo:\"" << obj_path.string() << "\" ";

        ss << "/link ";
        // Handshake symbols
        ss << "/EXPORT:tachyon_jit_build_scene /EXPORT:tachyon_jit_get_manifest ";
        
        for (const auto& l : cfg.link_libraries) ss << "\"" << l << "\" ";
#else
        ss << cfg.compiler << " -O3 -shared -fPIC -std=c++" << cfg.cxx_standard << " -DTACHYON_USE_DLL ";
        for (const auto& d : cfg.compile_definitions) ss << "-D" << d << " ";
        for (const auto& i : cfg.include_dirs) ss << "-I\"" << i << "\" ";
        ss << "\"" << cpp_path.string() << "\" ";
        ss << "-o \"" << dll_path.string() << "\" ";
        
        ss << "-Wl,--whole-archive ";
        for (const auto& l : cfg.link_libraries) {
            ss << "\"" << l << "\" ";
        }
        ss << "-Wl,--no-whole-archive ";
#endif
        return ss.str();
    }

    // Fallback to legacy behavior if JSON not found
#ifdef _WIN32
    if (const auto vcvars = find_vcvars64_bat(); vcvars.has_value()) {
        ss << "call \"" << vcvars->string() << "\" >nul && ";
    }
    // MSVC cl.exe command
    ss << "cl.exe /nologo /O2 /MD /EHsc /LD /std:c++20 /openmp /DNDEBUG /DTACHYON_JIT_EXPORTS /wd4190 ";
    ss << "/I\"" << TACHYON_INCLUDE_DIR << "\" ";
    ss << "\"" << cpp_path.string() << "\" ";
    ss << "/Fe:\"" << dll_path.string() << "\" ";
    auto obj_path = dll_path;
    obj_path.replace_extension(".obj");
    ss << "/Fo:\"" << obj_path.string() << "\" ";

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

    ss << "/link /EXPORT:tachyon_jit_build_scene /EXPORT:tachyon_jit_get_manifest ";
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
    const std::filesystem::path lib_root = TACHYON_LIB_PATH;
    
    for (std::size_t i = 0; i < libs.size(); ++i) {
        if (i != 0) ss << ' ';
        ss << "-L\"" << lib_root.string() << "\" -l" << libs[i] << " ";
    }
#endif
    return ss.str();
}

std::string AuthoringService::compute_cache_hash(const std::string& content) {
    uint64_t hash_val = 0xcbf29ce484222325ULL;
    
    auto hash_string = [&](const std::string& s) {
        for (char c : s) {
            hash_val ^= static_cast<uint8_t>(c);
            hash_val *= 0x100000001b3ULL;
        }
    };

    hash_string(content);
    hash_string(std::to_string(TACHYON_JIT_ABI_VERSION));
    hash_string(TACHYON_INCLUDE_DIR);
    hash_string(TACHYON_LIB_PATH);

    // Add TachyonScene library fingerprint to hash
    const std::vector<std::string> libs = get_link_libs();
    std::filesystem::path lib_dir = TACHYON_LIB_PATH;
    for (const auto& lib_name : libs) {
#ifdef _WIN32
        std::filesystem::path lp = lib_dir / (lib_name + ".lib");
#else
        std::filesystem::path lp = lib_dir / ("lib" + lib_name + ".so");
        if (!std::filesystem::exists(lp)) lp = lib_dir / ("lib" + lib_name + ".a");
#endif
        if (std::filesystem::exists(lp)) {
            hash_string(std::to_string(std::filesystem::file_size(lp)));
            hash_string(std::to_string(std::filesystem::last_write_time(lp).time_since_epoch().count()));
        }
    }

    std::stringstream hss;
    hss << std::hex << hash_val;
    return hss.str();
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
            hash_str = compute_cache_hash(content);
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
