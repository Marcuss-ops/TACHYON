#include "tachyon/core/spec/template_registry.h"
#include <iostream>

namespace tachyon {

std::shared_ptr<SceneSpec> TemplateRegistry::get_template(const std::filesystem::path& path) {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    std::string absolute_path = std::filesystem::absolute(path).string();
    
    auto it = m_cache.find(absolute_path);
    if (it != m_cache.end()) {
        return it->second;
    }

    // Load and parse the template
    auto result = parse_scene_spec_file(path);
    if (!result.ok()) {
        std::lock_guard<std::mutex> lock(m_mutex);
        for (const auto& diag : result.diagnostics.diagnostics) {
            // Se c'è un logger, stamperemo qui l'errore.
        }
        return nullptr;
    }

    auto template_ptr = std::make_shared<SceneSpec>(std::move(*result.value));
    m_cache[absolute_path] = template_ptr;
    
    return template_ptr;
}

void TemplateRegistry::clear_cache() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_cache.clear();
}

bool TemplateRegistry::is_cached(const std::filesystem::path& path) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    std::string absolute_path = std::filesystem::absolute(path).string();
    return m_cache.find(absolute_path) != m_cache.end();
}

} // namespace tachyon
