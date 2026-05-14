#pragma once

#include "tachyon/core/transition/transition_descriptor.h"
#include <memory>
#include <string>
#include <string_view>
#include <vector>
#include <unordered_map>
#include <stdexcept>

namespace tachyon {

/**
 * @brief Policy for handling duplicate registrations in the registry.
 */
enum class RegistryDuplicatePolicy {
    Reject,  ///< Throw an error on duplicate (debug/test)
    Warn,    ///< Log a warning and keep the first registration (permissive production)
    Replace  ///< Replace the existing entry with the new one
};

/**
 * @brief Exception thrown when a duplicate is rejected.
 */
class RegistryError : public std::runtime_error {
public:
    explicit RegistryError(const std::string& msg) : std::runtime_error(msg) {}
};

/**
 * Single unified registry for all transitions.
 * Replaces TransitionDescriptorRegistry, old TransitionRegistry, and TachyonLibrary.
 */
class TransitionRegistry {
public:
    /// Set the duplicate handling policy
    void set_duplicate_policy(RegistryDuplicatePolicy policy) { m_duplicate_policy = policy; }
    RegistryDuplicatePolicy duplicate_policy() const { return m_duplicate_policy; }

    /// Register a transition descriptor (single source of truth)
    void register_descriptor(const TransitionDescriptor& descriptor);

    /// Find by canonical ID
    const TransitionDescriptor* find_by_id(std::string_view id) const;

    /// Find by alias (alternative ID)
    const TransitionDescriptor* find_by_alias(std::string_view alias) const;

    /// Resolve by ID or alias (tries ID first, then alias)
    const TransitionDescriptor* resolve(std::string_view id_or_alias) const;

    /// Get library entries derived from registered descriptors
    std::vector<TransitionLibraryEntry> library_entries() const;

    /// List all registered descriptor IDs
    std::vector<std::string> list_all_ids() const;

    /// List all registered descriptors
    std::vector<const TransitionDescriptor*> list_all() const;

    /// Remove a registered transition by ID
    void unregister_transition(std::string_view id);

    TransitionRegistry();
    ~TransitionRegistry();

    // Non-copyable
    TransitionRegistry(const TransitionRegistry&) = delete;
    TransitionRegistry& operator=(const TransitionRegistry&) = delete;

    // Movable
    TransitionRegistry(TransitionRegistry&&) noexcept;
    TransitionRegistry& operator=(TransitionRegistry&&) noexcept;

private:
    RegistryDuplicatePolicy m_duplicate_policy{RegistryDuplicatePolicy::Warn}; ///< Default: warn on duplicates
    struct Impl;
    std::unique_ptr<Impl> m_impl;
};

/**
 * @brief Registers all built-in transitions.
 */
void register_builtin_transitions(TransitionRegistry& registry);

/**
 * @brief Creates and fully initializes a default transition registry with built-in implementations.
 */
TransitionRegistry create_default_transition_registry();

} // namespace tachyon
