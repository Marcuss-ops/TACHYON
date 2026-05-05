#include "tachyon/core/spec/cpp_scene_loader.h"
#include "tachyon/core/spec/authoring_service.h"
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

struct LoadedLib {
    void* handle{nullptr};
    BuildSceneFunc build_fn{nullptr};
    std::string error;
};

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

    using namespace tachyon::core::spec;
    const auto compiled = AuthoringService::get_instance().compile_to_shared_lib(cpp_path, output_dir);
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
    return core::spec::AuthoringService::get_instance().get_compiler_command(cpp_path, dll_path);
}

} // namespace tachyon
