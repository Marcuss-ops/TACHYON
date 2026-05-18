#include "tachyon/core/cli.h"
#include "tachyon/core/cli_options.h"
#include "cli/commands/command.h"
#include "cli/support/cli_internal.h"
#include "tachyon/runtime/plugins/plugin_abi.h"
#include "tachyon/runtime/plugins/plugin_manager.h"
#include "tachyon/transition_registry.h"

#include <iostream>
#include <string>
#include <vector>
#include <filesystem>
#include <fstream>
#include <memory>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#else
#include <dlfcn.h>
#endif

namespace fs = std::filesystem;

namespace tachyon {

namespace {

struct RegisteredTransition {
    std::string id;
    std::string display_name;
};

// Thread-local or static context to capture registered transitions during validation
static std::vector<RegisteredTransition>* g_validating_transitions = nullptr;

extern "C" {
    static int validate_register_transition(
        void* /*registry_handle*/,
        const char* id,
        const char* display_name,
        TachyonPluginCpuTransitionFn /*cpu_fn*/
    ) {
        if (id && g_validating_transitions) {
            g_validating_transitions->push_back({id, display_name ? display_name : id});
        }
        return 0;
    }

    static TachyonPluginColor validate_get_pixel(TachyonPluginSurfaceHandle /*surface*/, uint32_t /*x*/, uint32_t /*y*/) {
        return TachyonPluginColor{0.0f, 0.0f, 0.0f, 0.0f};
    }

    static uint32_t validate_get_width(TachyonPluginSurfaceHandle /*surface*/) {
        return 0;
    }

    static uint32_t validate_get_height(TachyonPluginSurfaceHandle /*surface*/) {
        return 0;
    }
}

bool validate_single_plugin(const fs::path& path, std::ostream& out, std::ostream& err) {
    if (!fs::exists(path)) {
        err << "unsupported plugin api version: plugin file not found: " << path.string() << "\n";
        return false;
    }

    void* handle = nullptr;
#ifdef _WIN32
    handle = LoadLibraryW(path.wstring().c_str());
#else
    handle = dlopen(path.string().c_str(), RTLD_NOW | RTLD_GLOBAL);
#endif

    if (!handle) {
        std::string dl_err;
#ifdef _WIN32
        dl_err = std::to_string(GetLastError());
#else
        dl_err = dlerror();
#endif
        err << "unsupported plugin api version: failed to load dynamic library: " << dl_err << "\n";
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
        err << "unsupported plugin api version: missing entry symbols 'tachyon_plugin_init' or 'tachyon_plugin_get_manifest'\n";
#ifdef _WIN32
        FreeLibrary((HMODULE)handle);
#else
        dlclose(handle);
#endif
        return false;
    }

    const auto* manifest = get_manifest_fn();
    if (!manifest) {
        err << "unsupported plugin api version: failed to retrieve plugin manifest\n";
#ifdef _WIN32
        FreeLibrary((HMODULE)handle);
#else
        dlclose(handle);
#endif
        return false;
    }

    // Call init with our custom API struct to collect exported transitions
    std::vector<RegisteredTransition> transitions;
    g_validating_transitions = &transitions;

    TachyonPluginApi api = {};
    api.abi_version = TACHYON_PLUGIN_ABI_VERSION; // 1
    api.registry_handle = nullptr;
    api.register_transition = validate_register_transition;
    api.get_pixel = validate_get_pixel;
    api.get_width = validate_get_width;
    api.get_height = validate_get_height;

    int init_res = init_fn(&api);
    g_validating_transitions = nullptr;

    if (init_res != 0) {
        err << "unsupported plugin api version: initialization returned error: " << init_res << "\n";
#ifdef _WIN32
        FreeLibrary((HMODULE)handle);
#else
        dlclose(handle);
#endif
        return false;
    }

    out << "Plugin Validation: SUCCESS\n";
    out << "  Name:         " << (manifest->name ? manifest->name : "N/A") << "\n";
    out << "  Author:       " << (manifest->author ? manifest->author : "N/A") << "\n";
    out << "  Description:  " << (manifest->description ? manifest->description : "N/A") << "\n";
    out << "  Version:      " << manifest->version_major << "." 
        << manifest->version_minor << "." 
        << manifest->version_patch << "\n";
    out << "  Transitions:  [" << transitions.size() << "]\n";
    for (const auto& trans : transitions) {
        out << "    - " << trans.id << " (" << trans.display_name << ")\n";
    }

#ifdef _WIN32
    FreeLibrary((HMODULE)handle);
#else
    dlclose(handle);
#endif
    return true;
}

bool list_plugins_in_dir(const fs::path& dir, std::ostream& out, std::ostream& err) {
    if (!fs::exists(dir)) {
        err << "[ERROR] Plugin directory does not exist: " << dir.string() << "\n";
        return false;
    }

    out << "Scanning directory for plugins: " << dir.string() << "\n\n";

    int found_count = 0;
    for (const auto& entry : fs::directory_iterator(dir)) {
        if (!entry.is_regular_file()) continue;
        auto path = entry.path();
        auto ext = path.extension().string();
        
        if (ext == ".so" || ext == ".dll" || ext == ".dylib") {
            found_count++;
            out << "[" << found_count << "] File: " << path.filename().string() << "\n";
            
            // Try loading and reading metadata gracefully
            void* handle = nullptr;
#ifdef _WIN32
            handle = LoadLibraryW(path.wstring().c_str());
#else
            handle = dlopen(path.string().c_str(), RTLD_NOW | RTLD_GLOBAL);
#endif
            if (!handle) {
                out << "    Status: FAILED TO LOAD (Not a valid shared library or dynamic link failure)\n\n";
                continue;
            }

            auto get_manifest_fn = (TachyonPluginGetManifestFunc)
#ifdef _WIN32
                GetProcAddress((HMODULE)handle, "tachyon_plugin_get_manifest");
#else
                dlsym(handle, "tachyon_plugin_get_manifest");
#endif

            if (!get_manifest_fn) {
                out << "    Status: INVALID PLUGIN (Missing 'tachyon_plugin_get_manifest' symbol)\n\n";
#ifdef _WIN32
                FreeLibrary((HMODULE)handle);
#else
                dlclose(handle);
#endif
                continue;
            }

            const auto* manifest = get_manifest_fn();
            if (!manifest) {
                out << "    Status: INVALID PLUGIN (Failed to retrieve manifest)\n\n";
#ifdef _WIN32
                FreeLibrary((HMODULE)handle);
#else
                dlclose(handle);
#endif
                continue;
            }

            out << "    Name:        " << (manifest->name ? manifest->name : "N/A") << "\n";
            out << "    Author:      " << (manifest->author ? manifest->author : "N/A") << "\n";
            out << "    Description: " << (manifest->description ? manifest->description : "N/A") << "\n";
            out << "    Version:     " << manifest->version_major << "." 
                << manifest->version_minor << "." 
                << manifest->version_patch << "\n";
            out << "    Status:      VALID\n\n";

#ifdef _WIN32
            FreeLibrary((HMODULE)handle);
#else
            dlclose(handle);
#endif
        }
    }

    if (found_count == 0) {
        out << "No dynamic plugins found in directory.\n";
    } else {
        out << "Total plugins found: " << found_count << "\n";
    }

    return true;
}

bool reload_plugins_in_dir(const fs::path& dir, std::ostream& out, std::ostream& err) {
    if (!fs::exists(dir)) {
        err << "[ERROR] Plugin directory does not exist: " << dir.string() << "\n";
        return false;
    }

    out << "Reloading plugins from: " << dir.string() << "\n";
    
    // 1. Unload all loaded plugins
    runtime::PluginManager::get_instance().unload_all();
    out << "Unloaded all existing plugins successfully.\n";

    // 2. Scan and load all dynamic libraries in directory
    TransitionRegistry registry;
    int reload_count = 0;
    
    for (const auto& entry : fs::directory_iterator(dir)) {
        if (!entry.is_regular_file()) continue;
        auto path = entry.path();
        auto ext = path.extension().string();
        
        if (ext == ".so" || ext == ".dll" || ext == ".dylib") {
            bool loaded = runtime::PluginManager::get_instance().load_plugin(path, registry);
            if (loaded) {
                reload_count++;
            } else {
                err << "  [WARNING] Failed to load plugin: " << path.filename().string() << "\n";
            }
        }
    }

    out << "Successfully reloaded " << reload_count << " plugins.\n";
    return true;
}

} // namespace

bool run_plugins_command(const CliOptions& options, std::ostream& out, std::ostream& err, runtime::EngineRegistry& /*bundle*/) {
    std::string sub = options.plugins.subcommand;

    if (sub == "list") {
        return list_plugins_in_dir(options.plugins.dir, out, err);
    } else if (sub == "validate") {
        if (options.plugins.path.empty()) {
            err << "[ERROR] Missing required option --path for plugin validation.\n";
            return false;
        }
        return validate_single_plugin(options.plugins.path, out, err);
    } else if (sub == "reload") {
        return reload_plugins_in_dir(options.plugins.dir, out, err);
    } else {
        err << "[ERROR] Unknown plugins subcommand: " << sub << "\n";
        err << "Available subcommands: list, validate, reload\n";
        return false;
    }
}

CommandDescriptor make_plugins_command() {
    return {
        "plugins",
        "tachyon plugins list [--dir <dir>]\n"
        "tachyon plugins validate --path <path>\n"
        "tachyon plugins reload [--dir <dir>]",
        nullptr,
        run_plugins_command
    };
}

} // namespace tachyon
