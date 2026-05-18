#include "tachyon/runtime/plugins/plugin_manager.h"
#include "tachyon/runtime/plugins/plugin_abi.h"
#include "tachyon/renderer2d/core/framebuffer.h"
#include "tachyon/core/transition/transition_descriptor.h"

#include <iostream>
#include <mutex>
#include <array>
#include <unordered_map>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#else
#include <dlfcn.h>
#endif

namespace tachyon::runtime {

namespace {

constexpr size_t MAX_PLUGIN_TRANSITIONS = 32;
static std::array<TachyonPluginCpuTransitionFn, MAX_PLUGIN_TRANSITIONS> s_plugin_slots{};
static std::array<std::string, MAX_PLUGIN_TRANSITIONS> s_plugin_slot_ids{};
static size_t s_next_slot = 0;
static std::mutex s_slot_mutex;

// Dynamic wrapper executing plugin code safely inside Tachyon pipeline
template<size_t Index>
renderer2d::Color plugin_cpu_wrapper(
    float u, float v, float t,
    const renderer2d::SurfaceRGBA& input,
    const renderer2d::SurfaceRGBA* to_surface
) {
    TachyonPluginCpuTransitionFn fn = s_plugin_slots[Index];
    if (!fn) {
        return renderer2d::Color::transparent();
    }
    
    TachyonPluginSurfaceHandle input_h = const_cast<renderer2d::SurfaceRGBA*>(&input);
    TachyonPluginSurfaceHandle to_h = const_cast<renderer2d::SurfaceRGBA*>(to_surface);
    TachyonPluginColor c = fn(u, v, t, input_h, to_h);
    return renderer2d::Color{c.r, c.g, c.b, c.a};
}

#define WRAPPER_SLOT(idx) &plugin_cpu_wrapper<idx>
static constexpr std::array<CpuTransitionFn, MAX_PLUGIN_TRANSITIONS> s_wrapper_fns = {
    WRAPPER_SLOT(0),  WRAPPER_SLOT(1),  WRAPPER_SLOT(2),  WRAPPER_SLOT(3),
    WRAPPER_SLOT(4),  WRAPPER_SLOT(5),  WRAPPER_SLOT(6),  WRAPPER_SLOT(7),
    WRAPPER_SLOT(8),  WRAPPER_SLOT(9),  WRAPPER_SLOT(10), WRAPPER_SLOT(11),
    WRAPPER_SLOT(12), WRAPPER_SLOT(13), WRAPPER_SLOT(14), WRAPPER_SLOT(15),
    WRAPPER_SLOT(16), WRAPPER_SLOT(17), WRAPPER_SLOT(18), WRAPPER_SLOT(19),
    WRAPPER_SLOT(20), WRAPPER_SLOT(21), WRAPPER_SLOT(22), WRAPPER_SLOT(23),
    WRAPPER_SLOT(24), WRAPPER_SLOT(25), WRAPPER_SLOT(26), WRAPPER_SLOT(27),
    WRAPPER_SLOT(28), WRAPPER_SLOT(29), WRAPPER_SLOT(30), WRAPPER_SLOT(31)
};

// C ABI safe helper implementation
extern "C" {
    static TachyonPluginColor host_get_pixel(TachyonPluginSurfaceHandle surface, uint32_t x, uint32_t y) {
        if (!surface) return {0.0f, 0.0f, 0.0f, 0.0f};
        const auto* s = static_cast<const renderer2d::SurfaceRGBA*>(surface);
        renderer2d::Color c = s->get_pixel(x, y);
        return TachyonPluginColor{c.r, c.g, c.b, c.a};
    }

    static uint32_t host_get_width(TachyonPluginSurfaceHandle surface) {
        if (!surface) return 0;
        return static_cast<const renderer2d::SurfaceRGBA*>(surface)->width();
    }

    static uint32_t host_get_height(TachyonPluginSurfaceHandle surface) {
        if (!surface) return 0;
        return static_cast<const renderer2d::SurfaceRGBA*>(surface)->height();
    }

    static int host_register_transition(
        void* registry_handle,
        const char* id,
        const char* display_name,
        TachyonPluginCpuTransitionFn cpu_fn
    ) {
        if (!registry_handle || !id || !cpu_fn) return -1;
        auto* registry = static_cast<TransitionRegistry*>(registry_handle);

        std::lock_guard<std::mutex> lock(s_slot_mutex);
        if (s_next_slot >= MAX_PLUGIN_TRANSITIONS) {
            std::cerr << "[Tachyon][Plugin] Error: Exceeded maximum plugin transitions limits" << std::endl;
            return -2;
        }

        size_t assigned_slot = s_next_slot++;
        s_plugin_slots[assigned_slot] = cpu_fn;
        s_plugin_slot_ids[assigned_slot] = id;

        TransitionDescriptor desc;
        desc.id = id;
        desc.display_name = display_name ? display_name : id;
        desc.description = "Dynamic plugin transition";
        desc.runtime_kind = TransitionRuntimeKind::CpuPixel;
        desc.category = TransitionCategory::Fade;
        desc.capabilities.supports_cpu = true;
        desc.cpu_fn = s_wrapper_fns[assigned_slot];

        registry->register_descriptor(desc);
        return 0;
    }
}

} // namespace

struct PluginManager::Impl {
    struct LoadedPlugin {
        std::filesystem::path path;
        void* handle{nullptr};
        std::string name;
    };
    std::vector<LoadedPlugin> plugins;
};

PluginManager& PluginManager::get_instance() {
    static PluginManager instance;
    return instance;
}

PluginManager::~PluginManager() {
    unload_all();
}

bool PluginManager::load_plugin(const std::filesystem::path& path, TransitionRegistry& registry) {
    if (!std::filesystem::exists(path)) {
        std::cerr << "[Tachyon][Plugin] Error: Plugin file not found: " << path << std::endl;
        return false;
    }

    if (!m_impl) {
        m_impl = std::make_unique<Impl>();
    }

    void* handle = nullptr;
#ifdef _WIN32
    handle = LoadLibraryW(path.wstring().c_str());
#else
    handle = dlopen(path.string().c_str(), RTLD_NOW | RTLD_GLOBAL);
#endif

    if (!handle) {
        std::string err;
#ifdef _WIN32
        err = std::to_string(GetLastError());
#else
        err = dlerror();
#endif
        std::cerr << "[Tachyon][Plugin] Error: Failed to dynamic link plugin library: " << err << std::endl;
        return false;
    }

    TachyonPluginInitFunc init_fn = nullptr;
    TachyonPluginGetManifestFunc get_manifest_fn = nullptr;

#ifdef _WIN32
    init_fn = (TachyonPluginInitFunc)GetProcAddress((HMODULE)handle, "tachyon_plugin_init");
    get_manifest_fn = (TachyonPluginGetManifestFunc)GetProcAddress((HMODULE)handle, "tachyon_plugin_get_manifest");
#else
    init_fn = (TachyonPluginInitFunc)dlsym(handle, "tachyon_plugin_init");
    get_manifest_fn = (TachyonPluginGetManifestFunc)dlsym(handle, "tachyon_plugin_get_manifest");
#endif

    if (!init_fn || !get_manifest_fn) {
        std::cerr << "[Tachyon][Plugin] Error: Missing entry symbols 'tachyon_plugin_init' or 'tachyon_plugin_get_manifest'" << std::endl;
#ifdef _WIN32
        FreeLibrary((HMODULE)handle);
#else
        dlclose(handle);
#endif
        return false;
    }

    const auto* manifest = get_manifest_fn();
    if (!manifest) {
        std::cerr << "[Tachyon][Plugin] Error: Failed to retrieve plugin manifest" << std::endl;
#ifdef _WIN32
        FreeLibrary((HMODULE)handle);
#else
        dlclose(handle);
#endif
        return false;
    }

    TachyonPluginApi api = {};
    api.abi_version = TACHYON_PLUGIN_ABI_VERSION;
    api.registry_handle = &registry;
    api.register_transition = host_register_transition;
    api.get_pixel = host_get_pixel;
    api.get_width = host_get_width;
    api.get_height = host_get_height;

    int init_res = init_fn(&api);
    if (init_res != 0) {
        std::cerr << "[Tachyon][Plugin] Error: Plugin initialization returned error: " << init_res << std::endl;
#ifdef _WIN32
        FreeLibrary((HMODULE)handle);
#else
        dlclose(handle);
#endif
        return false;
    }

    Impl::LoadedPlugin lp;
    lp.path = path;
    lp.handle = handle;
    lp.name = manifest->name;
    m_impl->plugins.push_back(lp);

    std::cout << "[Tachyon][Plugin] Successfully loaded plugin '" << lp.name << "' from: " << path.filename().string() << std::endl;
    return true;
}

void PluginManager::unload_all() {
    if (!m_impl) return;
    for (auto& p : m_impl->plugins) {
        if (p.handle) {
#ifdef _WIN32
            FreeLibrary((HMODULE)p.handle);
#else
            dlclose(p.handle);
#endif
        }
    }
    m_impl->plugins.clear();

    std::lock_guard<std::mutex> lock(s_slot_mutex);
    s_plugin_slots.fill(nullptr);
    s_plugin_slot_ids.fill("");
    s_next_slot = 0;
}

std::vector<std::string> PluginManager::list_loaded_plugins() const {
    std::vector<std::string> list;
    if (!m_impl) return list;
    for (const auto& p : m_impl->plugins) {
        list.push_back(p.name);
    }
    return list;
}

} // namespace tachyon::runtime
