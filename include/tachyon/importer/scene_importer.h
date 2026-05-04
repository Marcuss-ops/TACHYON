#pragma once

#include "tachyon/core/spec/schema/objects/scene_spec.h"
#include <memory>
#include <string>

namespace tachyon::importer {

// ---------------------------------------------------------------------------
// SceneImporter – abstract interface for external scene formats.
//
// Usage:
//   auto importer = create_usd_importer();        // or create_alembic_importer()
//   if (!importer || !importer->load(path)) { ... importer->last_error() ... }
//   SceneSpec spec = importer->build_scene_spec();
//   // Hand spec to SceneCompiler as usual.
// ---------------------------------------------------------------------------
class SceneImporter {
public:
    virtual ~SceneImporter() = default;

    // Load the scene from disk. Returns false on failure.
    virtual bool load(const std::string& path) = 0;

    // Build a Tachyon SceneSpec from the loaded data.
    // May be called multiple times (e.g. for different frame ranges).
    virtual SceneSpec build_scene_spec() const = 0;

    // Query animated data at a specific time (seconds).
    // Returns a SceneSpec with transforms evaluated at t.
    virtual SceneSpec build_scene_spec_at(double time_seconds) const {
        (void)time_seconds;
        return build_scene_spec();  // default: static
    }

    // Frame range of the loaded file (in scene time).
    virtual double start_time_seconds() const { return 0.0; }
    virtual double end_time_seconds()   const { return 0.0; }

    // True if the scene contains animated data.
    virtual bool is_animated() const { return false; }

    virtual std::string last_error() const = 0;
};

class SceneImporterBase : public SceneImporter {
public:
    double start_time_seconds() const override { return m_start_time; }
    double end_time_seconds()   const override { return m_end_time; }
    bool   is_animated()        const override { return m_is_animated; }
    std::string last_error()    const override { return m_error; }
protected:
    double m_start_time{0.0};
    double m_end_time{0.0};
    bool   m_is_animated{false};
    mutable std::string m_error;
};

// Factories – return nullptr if the respective format is not compiled in
TACHYON_API std::unique_ptr<SceneImporter> create_usd_importer();
TACHYON_API std::unique_ptr<SceneImporter> create_alembic_importer();

} // namespace tachyon::importer
