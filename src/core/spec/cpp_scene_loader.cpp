#include "tachyon/core/spec/cpp_scene_loader.h"
#include "tachyon/tachyon_build_config.h"
#include "tachyon/core/platform/process.h"
#include <cstdlib>
#include <iostream>
#include <fstream>
#include <sstream>
#include <array>
#include <optional>
#include <vector>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#else
#include <dlfcn.h>
#endif

namespace tachyon {

namespace {

using BuildSceneFunc = void(*)(SceneSpec&);

struct CompileResult {
    bool success{false};
    std::string error;
    std::filesystem::path output_path;
};

struct LoadedLib {
    void* handle{nullptr};
    BuildSceneFunc build_fn{nullptr};
    std::string error;
};

bool msvc_environment_is_ready() {
#ifdef _WIN32
    return std::getenv("VSCMD_VER") ||
           std::getenv("VCINSTALLDIR") ||
           std::getenv("VCToolsInstallDir");
#else
    return true;
#endif
}

std::optional<std::filesystem::path> find_vswhere_exe() {
#ifdef _WIN32
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
#endif
    return std::nullopt;
}

std::optional<std::filesystem::path> find_vcvars64_bat() {
#ifndef _WIN32
    return std::nullopt;
#else
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
#endif
}

std::vector<std::string> scene_loader_link_libs() {
    return {
        "TachyonCore",
        "TachyonScene",
        "TachyonRenderer2D",
        "TachyonRenderer3D",
        "TachyonColor",
        "TachyonPlatform",
        "TachyonAudio",
        "TachyonMedia",
        "TachyonOutput",
        "TachyonText"
    };
}

std::string build_link_lib_list(const std::filesystem::path& lib_path) {
    std::ostringstream libs;
    const auto names = scene_loader_link_libs();
    for (std::size_t i = 0; i < names.size(); ++i) {
        if (i != 0) {
            libs << ' ';
        }
        libs << "\"" << (lib_path / (names[i] + ".lib")).string() << "\"";
    }
    return libs.str();
}

std::string build_unix_link_lib_list(const std::filesystem::path& lib_path) {
    std::ostringstream libs;
    const auto names = scene_loader_link_libs();
    for (std::size_t i = 0; i < names.size(); ++i) {
        if (i != 0) {
            libs << ' ';
        }
        libs << "\"" << (lib_path / ("lib" + names[i] + ".a")).string() << "\"";
    }
    return libs.str();
}

CompileResult compile_scene_to_shared_lib(
    const std::filesystem::path& cpp_path,
    const std::filesystem::path& output_dir) {
    
    CompileResult result;
    std::filesystem::path dll_path = output_dir / cpp_path.filename();
#ifdef _WIN32
    dll_path.replace_extension(".dll");
#else
    dll_path.replace_extension(".so");
#endif
    std::filesystem::create_directories(output_dir);

#ifdef _WIN32
    if (!msvc_environment_is_ready() && !find_vcvars64_bat().has_value()) {
        result.error = "Visual Studio C++ build tools were not found. "
                       "Run the loader from a Developer Command Prompt or install VS Build Tools.";
        return result;
    }
#endif

    const std::string cmd = CppSceneLoader::get_compiler_command(cpp_path, dll_path);
    std::cout << "[CppSceneLoader] Compiling: " << cmd << std::endl;
    
    using namespace tachyon::core::platform;
    ProcessSpec spec;
#ifdef _WIN32
    spec.executable = "cmd.exe";
    spec.args = {"/C", cmd};
#else
    spec.executable = "sh";
    spec.args = {"-c", cmd};
#endif
    const auto proc = run_process(spec);
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

LoadedLib load_scene_library(const std::filesystem::path& dll_path) {
    LoadedLib lib;
#ifdef _WIN32
    HMODULE h = LoadLibraryW(dll_path.wstring().c_str());
    if (!h) { lib.error = "LoadLibrary failed: " + std::to_string(GetLastError()); return lib; }
    lib.build_fn = (BuildSceneFunc)GetProcAddress(h, "build_scene");
    lib.handle = h;
#else
    void* h = dlopen(dll_path.string().c_str(), RTLD_NOW);
    if (!h) { lib.error = std::string("dlopen: ") + dlerror(); return lib; }
    lib.build_fn = (BuildSceneFunc)dlsym(h, "build_scene");
    lib.handle = h;
#endif
    if (!lib.build_fn) lib.error = "Symbol 'build_scene' not found";
    return lib;
}

void unload_scene_library(LoadedLib& lib) {
    if (!lib.handle) {
        return;
    }
#ifdef _WIN32
    FreeLibrary(static_cast<HMODULE>(lib.handle));
#else
    dlclose(lib.handle);
#endif
    lib.handle = nullptr;
}

} // namespace

CppSceneLoader::Result CppSceneLoader::load_from_file(
    const std::filesystem::path& cpp_path,
    const std::filesystem::path& output_dir) {

    Result result;
    if (!std::filesystem::exists(cpp_path)) {
        result.diagnostics = "C++ file does not exist: " + cpp_path.string();
        return result;
    }

    const auto compiled = compile_scene_to_shared_lib(cpp_path, output_dir);
    if (!compiled.success) { result.diagnostics = compiled.error; return result; }

    auto lib = load_scene_library(compiled.output_path);
    if (!lib.build_fn) { result.diagnostics = lib.error; return result; }

    try {
        SceneSpec scene;
        lib.build_fn(scene);
        result.scene    = std::move(scene);
        result.success  = true;
    } catch (const std::exception& e) {
        result.diagnostics = std::string("build_scene() threw: ") + e.what();
    }
    unload_scene_library(lib);
    return result;
}

std::string CppSceneLoader::get_compiler_command(
    const std::filesystem::path& cpp_path,
    const std::filesystem::path& dll_path) {
    
    std::stringstream ss;
#ifdef _WIN32
    if (const auto vcvars = find_vcvars64_bat(); vcvars.has_value()) {
        ss << "call \"" << vcvars->string() << "\" >nul && ";
    }

    // MSVC cl.exe command
    ss << "cl.exe /nologo /O2 /MD /EHsc /LD /std:c++20 /DTACHYON_USE_DLL ";
    ss << "/I\"" << TACHYON_INCLUDE_DIR << "\" ";
    ss << "\"" << cpp_path.string() << "\" ";
    ss << "/Fe:\"" << dll_path.string() << "\" ";
    std::filesystem::path lib_path = TACHYON_LIB_PATH;
    if (!std::filesystem::exists(lib_path / TACHYON_CORE_LIB)) {
        if (std::filesystem::exists(lib_path / "RelWithDebInfo" / TACHYON_CORE_LIB)) {
            lib_path /= "RelWithDebInfo";
        } else if (std::filesystem::exists(lib_path / "Release" / TACHYON_CORE_LIB)) {
            lib_path /= "Release";
        } else if (std::filesystem::exists(lib_path / "Debug" / TACHYON_CORE_LIB)) {
            lib_path /= "Debug";
        }
    }

    ss << "/link /EXPORT:build_scene ";
    ss << build_link_lib_list(lib_path);
#else
    // Clang/GCC command
    ss << "c++ -O3 -shared -fPIC -std=c++20 -DTACHYON_USE_DLL ";
    ss << "-I\"" << TACHYON_INCLUDE_DIR << "\" ";
    ss << "\"" << cpp_path.string() << "\" ";
    ss << "-o \"" << dll_path.string() << "\" ";
    ss << build_unix_link_lib_list(TACHYON_LIB_PATH);
#endif
    return ss.str();
}

} // namespace tachyon
