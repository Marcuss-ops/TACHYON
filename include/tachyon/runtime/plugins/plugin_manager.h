#pragma once

#include "tachyon/transition_registry.h"
#include <string>
#include <vector>
#include <memory>
#include <filesystem>

namespace tachyon::runtime {

class PluginManager {
public:
    static PluginManager& get_instance();

    // Load a plugin dynamic library file and registers its features into the registry
    bool load_plugin(const std::filesystem::path& path, TransitionRegistry& registry);
    
    // Unloads all loaded plugins and cleans up resources
    void unload_all();

    // Returns a list of loaded plugin descriptions
    std::vector<std::string> list_loaded_plugins() const;

private:
    PluginManager() = default;
    ~PluginManager();
    
    PluginManager(const PluginManager&) = delete;
    PluginManager& operator=(const PluginManager&) = delete;

    struct Impl;
    std::unique_ptr<Impl> m_impl;
};

} // namespace tachyon::runtime
