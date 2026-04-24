#pragma once

#include "tachyon/core/spec/schema/objects/scene_spec.h"
#include <string>
#include <functional>
#include <map>

namespace tachyon::core {

/**
 * @brief Migration function type for upgrading scene specs between schema versions.
 */
using MigrationFunction = std::function<bool(SceneSpec&)>;

/**
 * @brief Manages schema migrations for SceneSpec objects.
 * 
 * Provides automatic migration from older schema versions to the current version.
 * Each migration function should return true on success, false on failure.
 */
class SceneMigrationManager {
public:
    SceneMigrationManager();
    ~SceneMigrationManager();

    /**
     * @brief Migrate a scene spec to the target schema version.
     * @param scene The scene spec to migrate (modified in-place).
     * @param target_version The target schema version (defaults to current).
     * @return true if migration succeeded, false otherwise.
     */
    bool migrate(SceneSpec& scene, const SchemaVersion& target_version = SchemaVersion{1, 0, 0});

    /**
     * @brief Register a migration function for a specific version transition.
     * @param from_version The source schema version.
     * @param to_version The target schema version.
     * @param migration_fn The migration function to apply.
     */
    void register_migration(const SchemaVersion& from_version, const SchemaVersion& to_version, MigrationFunction migration_fn);

    /**
     * @brief Get the current schema version supported by this manager.
     */
    static SchemaVersion current_version();

private:
    struct MigrationKey {
        SchemaVersion from;
        SchemaVersion to;
        
        bool operator<(const MigrationKey& other) const {
            if (from != other.from) return from < other.from;
            return to < other.to;
        }
    };
    
    std::map<MigrationKey, MigrationFunction> m_migrations;
};

} // namespace tachyon::core
