#include "tachyon/core/spec/schema/migration/scene_migration.h"
#include <algorithm>

namespace tachyon::core {

SceneMigrationManager::SceneMigrationManager() {
    // Register default migrations
    
    // Migration 0.9.x -> 1.0.0: Add schema_version field
    register_migration(
        SchemaVersion{0, 9, 0},
        SchemaVersion{1, 0, 0},
        [](SceneSpec& scene) -> bool {
            // Legacy files may not have schema_version, set it to 1.0.0
            if (scene.schema_version.major == 0 && scene.schema_version.minor == 0) {
                scene.schema_version = SchemaVersion{1, 0, 0};
            }
            return true;
        }
    );
}

SceneMigrationManager::~SceneMigrationManager() = default;

SchemaVersion SceneMigrationManager::current_version() {
    return SchemaVersion{1, 0, 0};
}

void SceneMigrationManager::register_migration(
    const SchemaVersion& from_version,
    const SchemaVersion& to_version,
    MigrationFunction migration_fn
) {
    MigrationKey key{from_version, to_version};
    m_migrations[key] = std::move(migration_fn);
}

bool SceneMigrationManager::migrate_in_place(SceneSpec& scene, const SchemaVersion& target_version) {
    SchemaVersion current = scene.schema_version;
    
    // Already at or beyond target version
    if (current >= target_version) {
        return true;
    }
    
    // Find and apply migrations iteratively
    bool progress = true;
    while (progress && current < target_version) {
        progress = false;
        
        // Find next applicable migration
        for (const auto& [key, fn] : m_migrations) {
            if (key.from == current && key.to <= target_version) {
                if (fn(scene)) {
                    scene.schema_version = key.to;
                    current = key.to;
                    progress = true;
                    break;
                } else {
                    // Migration failed
                    return false;
                }
            }
        }
        
        // No applicable migration found
        if (!progress && current < target_version) {
            return false;
        }
    }
    
    return scene.schema_version >= target_version;
}

std::optional<SceneSpec> SceneMigrationManager::migrate(const SceneSpec& scene, const SchemaVersion& target_version) {
    SceneSpec result = scene;
    if (migrate_in_place(result, target_version)) {
        return result;
    }
    return std::nullopt;
}

// Convenience function for migrating scenes
bool migrate_scene(SceneSpec& scene, const SchemaVersion& target_version) {
    static SceneMigrationManager manager;
    return manager.migrate_in_place(scene, target_version);
}

} // namespace tachyon::core
