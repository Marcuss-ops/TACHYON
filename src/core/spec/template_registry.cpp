#include "tachyon/core/spec/template_registry.h"
#include <iostream>

namespace tachyon {

std::shared_ptr<SceneSpec> TemplateRegistry::get_template(const std::filesystem::path& /*path*/) {
    std::cerr << "TemplateRegistry::get_template is DISABLED. JSON scene loading is no longer supported.\n";
    return nullptr;
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
