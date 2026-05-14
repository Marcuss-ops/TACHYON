#include "tachyon/core/spec/cpp_scene_loader.h"
#include "tachyon/core/spec/authoring_service.h"
#include "tachyon/core/platform/process.h"
#include "tachyon/jit/tachyon_jit_api.h"
#include "tachyon/scene/builder.h"
#include <cstdlib>
#include <iostream>
#include <fstream>
#include <sstream>
#include <array>
#include <optional>
#include <vector>
#include <map>
#include <memory>

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

using JitBuildSceneFunc = int(*)(const TachyonHostApi*, TachyonSceneHandle);

struct LoadedLib {
    void* handle{nullptr};
    JitBuildSceneFunc jit_build_fn{nullptr};
    BuildSceneFunc build_scene_v1_fn{nullptr};
    std::string error;
};

struct HostContext {
    scene::CompositionBuilder current_comp;
    std::map<TachyonLayerHandle, std::unique_ptr<scene::LayerBuilder>> layers;
    uint64_t next_layer_handle{1};

    HostContext() : current_comp(scene::Composition("main")) {
        current_comp.size(1920, 1080).fps(30).duration(10.0);
    }

    TachyonLayerHandle register_layer(std::unique_ptr<scene::LayerBuilder> lb) {
        TachyonLayerHandle h = next_layer_handle++;
        layers[h] = std::move(lb);
        return h;
    }
};

extern "C" {
    static void host_log(void* ctx, int level, const char* message) {
        // Silenced for production paths.
    }

    static int host_begin_scene(void* ctx, TachyonSceneHandle scene) {
        return TACHYON_JIT_OK;
    }

    static int host_end_scene(void* ctx, TachyonSceneHandle scene) {
        auto* host = static_cast<HostContext*>(ctx);
        // Build all layers into the composition
        for (auto& [h, lb] : host->layers) {
            host->current_comp.layer(lb->build());
        }
        host->layers.clear();
        return TACHYON_JIT_OK;
    }

    static TachyonLayerHandle host_add_background(void* ctx, TachyonSceneHandle scene, const char* background_id) {
        auto* host = static_cast<HostContext*>(ctx);
        
        static presets::EffectPresetRegistry empty_registry;
        auto lb = std::make_unique<scene::LayerBuilder>(background_id, empty_registry);
        lb->type(LayerType::Procedural);
        
        // Ensure procedural kind is set so it actually renders
        auto spec = lb->build();
        spec.procedural.emplace();
        spec.procedural->kind = background_id;
        lb->from_spec(spec);
        
        return host->register_layer(std::move(lb));
    }

    static TachyonLayerHandle host_add_text(void* ctx, TachyonSceneHandle scene, const char* text, float x, float y) {
        auto* host = static_cast<HostContext*>(ctx);

        static presets::EffectPresetRegistry empty_registry;
        auto lb = std::make_unique<scene::LayerBuilder>("text_layer", empty_registry);
        lb->type(LayerType::Text);
        lb->position(x, y);
        lb->text(text);
        return host->register_layer(std::move(lb));
    }

    static int host_set_float(void* ctx, TachyonObjectHandle object, const char* property, float value) {
        auto* host = static_cast<HostContext*>(ctx);
        auto it = host->layers.find(object);
        if (it == host->layers.end()) return TACHYON_JIT_ERROR_INVALID_ARGUMENT;

        std::string prop = property;
        if (prop == "opacity") {
            it->second->opacity(value);
        } else if (prop == "anchor_x") {
            float y = it->second->spec().transform.anchor_point.value ? it->second->spec().transform.anchor_point.value->y : 0.0f;
            it->second->anchor(value, y);
        } else if (prop == "anchor_y") {
            float x = it->second->spec().transform.anchor_point.value ? it->second->spec().transform.anchor_point.value->x : 0.0f;
            it->second->anchor(x, value);
        } else {
            return TACHYON_JIT_ERROR_UNSUPPORTED;
        }
        return TACHYON_JIT_OK;
    }
}

LoadedLib load_scene_library(const std::filesystem::path& dll_path) {
    LoadedLib lib;
#ifdef _WIN32
    HMODULE h = LoadLibraryW(dll_path.wstring().c_str());
    if (!h) { lib.error = "LoadLibrary failed: " + std::to_string(GetLastError()); return lib; }
    lib.jit_build_fn = (JitBuildSceneFunc)GetProcAddress(h, "tachyon_jit_build_scene");
    lib.build_scene_v1_fn = (BuildSceneFunc)GetProcAddress(h, "build_scene");
    lib.handle = h;
#else
    void* h = dlopen(dll_path.string().c_str(), RTLD_NOW);
    if (!h) { lib.error = std::string("dlopen: ") + dlerror(); return lib; }
    lib.jit_build_fn = (JitBuildSceneFunc)dlsym(h, "tachyon_jit_build_scene");
    lib.build_scene_v1_fn = (BuildSceneFunc)dlsym(h, "build_scene");
    lib.handle = h;
#endif
    if (!lib.jit_build_fn && !lib.build_scene_v1_fn) {
        lib.error = "No valid JIT entry point found (expected 'tachyon_jit_build_scene' or 'build_scene')";
    }
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
    if (!lib.jit_build_fn && !lib.build_scene_v1_fn) { result.diagnostics = lib.error; return result; }

    try {
        if (lib.jit_build_fn) {
            HostContext host_ctx;
            TachyonHostApi api = {};
            api.abi_version = TACHYON_JIT_ABI_VERSION;
            api.struct_size = sizeof(TachyonHostApi);
            api.ctx = &host_ctx;
            api.log = host_log;
            api.begin_scene = host_begin_scene;
            api.end_scene = host_end_scene;
            api.add_background = host_add_background;
            api.add_text = host_add_text;
            api.set_float = host_set_float;

            int jit_res = lib.jit_build_fn(&api, 0 /* Handle not used yet */);
            if (jit_res == TACHYON_JIT_OK) {
                result.scene = host_ctx.current_comp.build_scene();
                result.success = true;
            } else {
                result.diagnostics = "JIT build failed with error code: " + std::to_string(jit_res);
            }
        } else if (lib.build_scene_v1_fn) {
            result.scene = lib.build_scene_v1_fn();
            result.success = true;
        }
    } catch (const std::exception& e) {
        result.diagnostics = std::string("JIT execution threw: ") + e.what();
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
