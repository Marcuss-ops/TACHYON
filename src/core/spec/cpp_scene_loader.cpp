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

CppSceneLoader::Result CppSceneLoader::load_from_file(
    const std::filesystem::path& cpp_path,
    const std::filesystem::path& output_dir) {
    
    Result result;
    if (!std::filesystem::exists(cpp_path)) {
        result.diagnostics = "C++ file does not exist: " + cpp_path.string();
        return result;
    }

    if (!std::filesystem::exists(output_dir)) {
        std::filesystem::create_directories(output_dir);
    }

    std::filesystem::path dll_path = output_dir / cpp_path.filename();
#ifdef _WIN32
    dll_path.replace_extension(".dll");
#else
    dll_path.replace_extension(".so");
#endif

    // 1. Compile
    std::string cmd = get_compiler_command(cpp_path, dll_path);
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

    auto process_result = run_process(spec);
    if (!process_result.success || process_result.exit_code != 0) {
        result.diagnostics = "Compilation failed with exit code " + std::to_string(process_result.exit_code);
        return result;
    }

    // 2. Load
#ifdef _WIN32
    HMODULE handle = LoadLibraryA(dll_path.string().c_str());
    if (!handle) {
        result.diagnostics = "Failed to load DLL: " + dll_path.string() + " (Error: " + std::to_string(GetLastError()) + ")";
        return result;
    }

    auto build_scene_func = (BuildSceneFunc)GetProcAddress(handle, "build_scene");
    if (!build_scene_func) {
        result.diagnostics = "Symbol 'build_scene' not found in DLL";
        FreeLibrary(handle);
        return result;
    }
#else
    void* handle = dlopen(dll_path.string().c_str(), RTLD_NOW);
    if (!handle) {
        result.diagnostics = "Failed to load SO: " + std::string(dlerror());
        return result;
    }

    auto build_scene_func = (BuildSceneFunc)dlsym(handle, "build_scene");
    if (!build_scene_func) {
        result.diagnostics = "Symbol 'build_scene' not found in SO";
        dlclose(handle);
        return result;
    }
#endif

    // 3. Execute
    try {
        SceneSpec scene;
        build_scene_func(scene);
        result.scene = std::move(scene);
        result.success = true;
    } catch (const std::exception& e) {
        result.diagnostics = "Exception during build_scene(): " + std::string(e.what());
    }

    // Note: We don't unload the library yet because the SceneSpec might point to 
    // static data or types registered within the library. 
    // In a production environment, we'd need a lifetime management strategy.

    return result;
}

std::string CppSceneLoader::get_compiler_command(
    const std::filesystem::path& cpp_path,
    const std::filesystem::path& dll_path) {
    
    std::stringstream ss;
#ifdef _WIN32
    // MSVC cl.exe command
    ss << "cl.exe /nologo /O2 /LD /std:c++20 ";
    ss << "/I\"" << TACHYON_INCLUDE_DIR << "\" ";
    ss << "/I\"" << TACHYON_JSON_INCLUDE_DIR << "\" ";
    ss << "\"" << cpp_path.string() << "\" ";
    ss << "/Fe:\"" << dll_path.string() << "\" ";
    ss << "/link /LIBPATH:\"" << TACHYON_LIB_PATH << "\" ";
    ss << TACHYON_CORE_LIB;
#else
    // Clang/GCC command
    ss << "clang++ -O3 -shared -fPIC -std=c++20 ";
    ss << "-I\"" << TACHYON_INCLUDE_DIR << "\" ";
    ss << "-I\"" << TACHYON_JSON_INCLUDE_DIR << "\" ";
    ss << "\"" << cpp_path.string() << "\" ";
    ss << "-o \"" << dll_path.string() << "\" ";
    ss << "-L\"" << TACHYON_LIB_PATH << "\" ";
    ss << "-lTachyonCore";
#endif
    return ss.str();
}

} // namespace tachyon
