#include "tachyon/core/spec/cpp_scene_loader.h"
#include "tachyon/tachyon_build_config.h"
#include "tachyon/core/platform/process.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <array>

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
    result.success = true;
    result.output_path = dll_path;
    return result;
}

LoadedLib load_scene_library(const std::filesystem::path& dll_path) {
    LoadedLib lib;
#ifdef _WIN32
    HMODULE h = LoadLibraryA(dll_path.string().c_str());
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

    const auto lib = load_scene_library(compiled.output_path);
    if (!lib.build_fn) { result.diagnostics = lib.error; return result; }

    try {
        SceneSpec scene;
        lib.build_fn(scene);
        result.scene    = std::move(scene);
        result.success  = true;
    } catch (const std::exception& e) {
        result.diagnostics = std::string("build_scene() threw: ") + e.what();
    }
    return result;
}

std::string CppSceneLoader::get_compiler_command(
    const std::filesystem::path& cpp_path,
    const std::filesystem::path& dll_path) {
    
    std::stringstream ss;
#ifdef _WIN32
    // MSVC cl.exe command
    ss << "cl.exe /nologo /O2 /MD /EHsc /LD /std:c++20 ";
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

    ss << "/link /LIBPATH:\"" << lib_path.string() << "\" ";
    ss << TACHYON_CORE_LIB;
#else
    // Clang/GCC command
    ss << "clang++ -O3 -shared -fPIC -std=c++20 ";
    ss << "-I\"" << TACHYON_INCLUDE_DIR << "\" ";
    ss << "\"" << cpp_path.string() << "\" ";
    ss << "-o \"" << dll_path.string() << "\" ";
    ss << "-L\"" << TACHYON_LIB_PATH << "\" ";
    ss << "-lTachyonCore";
#endif
    return ss.str();
}

} // namespace tachyon
