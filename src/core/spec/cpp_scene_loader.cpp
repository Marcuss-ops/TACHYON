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
    std::vector<std::string> logs;

    HostContext() : current_comp(scene::Composition("main")) {
        current_comp.size(1920, 1080).fps(30).duration(10.0);
    }
    
    void log(int level, const std::string& msg) {
        std::string prefix;
        switch(level) {
            case TACHYON_JIT_LOG_ERROR: prefix = "[ERROR] "; break;
            case TACHYON_JIT_LOG_WARN:  prefix = "[WARN]  "; break;
            case TACHYON_JIT_LOG_INFO:  prefix = "[INFO]  "; break;
            case TACHYON_JIT_LOG_DEBUG: prefix = "[DEBUG] "; break;
            default: prefix = "[LOG]   "; break;
        }
        logs.push_back(prefix + msg);
        
        if (const char* v = std::getenv("TACHYON_JIT_VERBOSE"); v && v[0] != '0') {
            std::cerr << "  JIT: " << prefix << msg << std::endl;
        }
    }

    TachyonLayerHandle register_layer(std::unique_ptr<scene::LayerBuilder> lb) {
        TachyonLayerHandle h = next_layer_handle++;
        layers[h] = std::move(lb);
        return h;
    }
};

extern "C" {
    static void host_log(void* ctx, int level, const char* message) {
        if (ctx && message) {
            static_cast<HostContext*>(ctx)->log(level, message);
        }
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
        
        auto lb = std::make_unique<scene::LayerBuilder>(background_id);
        lb->type(LayerType::Procedural);
        
        // Ensure procedural kind is set so it actually renders
        auto spec = lb->build();
        ProceduralSpec proc;
        proc.kind = background_id;
        spec.source = ProceduralSource{proc.kind, proc};
        lb->from_spec(spec);
        
        return host->register_layer(std::move(lb));
    }

    static TachyonLayerHandle host_add_text(void* ctx, TachyonSceneHandle scene, const char* text, float x, float y) {
        auto* host = static_cast<HostContext*>(ctx);

        auto lb = std::make_unique<scene::LayerBuilder>("text_layer");
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
            float y = it->second->spec().transform.transform.anchor_point.value ? it->second->spec().transform.transform.anchor_point.value->y : 0.0f;
            it->second->anchor(value, y);
        } else if (prop == "anchor_y") {
            float x = it->second->spec().transform.transform.anchor_point.value ? it->second->spec().transform.transform.anchor_point.value->x : 0.0f;
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
    const std::filesystem::path& path,
    const std::filesystem::path& output_dir) {

    Result result;
    if (!std::filesystem::exists(path)) {
        result.diagnostics = "File does not exist: " + path.string();
        return result;
    }

    std::filesystem::path module_path = path;
    const std::string ext = path.extension().string();
    const bool is_source = (ext == ".cpp" || ext == ".cc" || ext == ".cxx");

    if (is_source) {
        using namespace tachyon::core::spec;
        const auto compiled = AuthoringService::get_instance().compile_to_shared_lib(path, output_dir);
        if (!compiled.success) { 
            result.diagnostics = compiled.error; 
            return result; 
        }
        module_path = compiled.output_path;
    }

    auto lib = load_scene_library(module_path);
    if (!lib.handle) {
        result.diagnostics = lib.error;
        return result;
    }

    try {
        // ABI Handshake
        typedef const TachyonJitManifest* (*GetManifestFunc)();
        GetManifestFunc get_manifest = nullptr;
#ifdef _WIN32
        get_manifest = (GetManifestFunc)GetProcAddress((HMODULE)lib.handle, "tachyon_jit_get_manifest");
#else
        get_manifest = (GetManifestFunc)dlsym(lib.handle, "tachyon_jit_get_manifest");
#endif

        if (get_manifest) {
            const auto* manifest = get_manifest();
            if (manifest->abi_version != TACHYON_JIT_ABI_VERSION) {
                result.diagnostics = "ABI mismatch: module version " + std::to_string(manifest->abi_version) + 
                                   ", engine version " + std::to_string(TACHYON_JIT_ABI_VERSION);
                unload_scene_library(lib);
                return result;
            }
        }

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

            int jit_res = lib.jit_build_fn(&api, 0);
            
            for (const auto& log_line : host_ctx.logs) {
                result.diagnostics += log_line + "\n";
            }

            if (jit_res == TACHYON_JIT_OK) {
                result.scene = host_ctx.current_comp.build_scene();
                result.success = true;
            } else {
                result.diagnostics = "JIT build failed with error code: " + std::to_string(jit_res);
            }
        } else if (lib.build_scene_v1_fn) {
            std::cerr << "[TACHYON][WARN] Scene uses legacy 'build_scene' entry point. Please migrate to 'tachyon_jit_build_scene' for full JIT API support." << std::endl;
            result.scene = lib.build_scene_v1_fn();
            result.success = true;
        } else {
            result.diagnostics = "No valid JIT entry point found (expected 'tachyon_jit_build_scene' or 'build_scene')";
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
