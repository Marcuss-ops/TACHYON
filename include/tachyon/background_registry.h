#pragma once

#include "tachyon/background_descriptor.h"
#include "tachyon/core/spec/schema/common/common_spec.h"
#include <memory>
#include <string>
#include <string_view>
#include <vector>
#include <unordered_map>

namespace tachyon {


/**
 * @brief Unified registry for all backgrounds.
 * Single source of truth for background descriptors.
 */
#if defined(_MSC_VER)
#pragma warning(push)
#pragma warning(disable: 4251)
#endif
class TACHYON_API BackgroundRegistry {
public:
    /// Get the singleton instance
    static BackgroundRegistry& instance();

    /// Register a background descriptor
    void register_descriptor(const BackgroundDescriptor& descriptor);

    /// Find by canonical ID
    const BackgroundDescriptor* find_by_id(std::string_view id) const;

    /// Find by alias (alternative ID)
    const BackgroundDescriptor* find_by_alias(std::string_view alias) const;

    /// Resolve by ID or alias (tries ID first, then alias)
    const BackgroundDescriptor* resolve(std::string_view id_or_alias) const;

    /// List all registered descriptor IDs
    std::vector<std::string> list_all_ids() const;

    /// List all registered descriptors
    std::vector<const BackgroundDescriptor*> list_all() const;

    /// Remove a registered background by ID
    void unregister_background(std::string_view id);

    /// Register all built-in backgrounds (basic + procedural)
    void register_all_builtins();

private:
    BackgroundRegistry();
    ~BackgroundRegistry();

    BackgroundRegistry(const BackgroundRegistry&) = delete;
    BackgroundRegistry& operator=(const BackgroundRegistry&) = delete;

    struct Impl;
    std::unique_ptr<Impl> m_impl;
};
#if defined(_MSC_VER)
#pragma warning(pop)
#endif

} // namespace tachyon
