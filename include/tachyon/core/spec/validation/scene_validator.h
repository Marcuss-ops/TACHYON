#pragma once

#include "tachyon/core/api.h"
#include "tachyon/core/spec/schema/objects/scene_spec.h"
#include "tachyon/core/spec/schema/properties/property_spec.h"
#include <string>
#include <vector>
#include <optional>

namespace tachyon::core {

struct ValidationIssue {
    enum class Severity { Warning, Error, Fatal };
    Severity severity;
    std::string path; // e.g., "scene.compositions[0].layers[2].transform.position"
    std::string message;
};

struct ValidationResult {
    bool is_valid() const { return fatal_count == 0 && error_count == 0; }
    
    std::vector<ValidationIssue> issues;
    std::size_t error_count{0};
    std::size_t fatal_count{0};
    std::size_t warning_count{0};
};

/**
 * @brief Validates scene specifications for structural integrity and reference consistency.
 * 
 * Validation checks include:
 * - Schema version compatibility
 * - Duplicate ID detection (layers, tracks, cameras, audio)
 * - Dangling reference detection (precomps, assets, matte sources)
 * - Matte dependency cycle detection
 * - Precomp reference cycle detection
 * - Overlapping camera cuts
 * - Track binding validation
 */
class TACHYON_API SceneValidator {
public:
    SceneValidator() = default;
    ~SceneValidator() = default;

    // Perform strict validation on a loaded SceneSpec.
    // Checks for circular dependencies, invalid references (e.g., missing precomps, missing assets),
    // out-of-bounds values, and structural integrity.
    ValidationResult validate(const ::tachyon::SceneSpec& scene) const;

private:
    void validate_schema_version(const ::tachyon::SceneSpec& scene, ValidationResult& out) const;
    void validate_composition(const ::tachyon::CompositionSpec& comp, const ::tachyon::SceneSpec& scene, ValidationResult& out) const;
    void validate_layer(const ::tachyon::LayerSpec& layer, const ::tachyon::CompositionSpec& comp, const ::tachyon::SceneSpec& scene, const std::string& path, ValidationResult& out) const;
    void validate_safe_area(const ::tachyon::LayerSpec& layer, const ::tachyon::CompositionSpec& comp, const std::string& path, ValidationResult& out) const;
    void validate_duplicate_ids(const ::tachyon::CompositionSpec& comp, ValidationResult& out) const;
    void validate_camera_cuts(const ::tachyon::CompositionSpec& comp, ValidationResult& out) const;
    void validate_track_bindings(const ::tachyon::LayerSpec& layer, const std::string& path, ValidationResult& out) const;
    void validate_keyframes(const ::tachyon::LayerSpec& layer, const std::string& path, ValidationResult& out) const;
    void validate_font_reference(const ::tachyon::LayerSpec& layer, const ::tachyon::SceneSpec& scene, const std::string& path, ValidationResult& out) const;
    void validate_file_reference(const ::tachyon::LayerSpec& layer, const ::tachyon::SceneSpec& scene, const std::string& path, ValidationResult& out) const;
    // Property validation hook removed: PropertySpec type no longer exists
    
    // Checks for circular precomp references
    void check_cycles(const ::tachyon::SceneSpec& scene, ValidationResult& out) const;

    // Validate audio file references exist
    void validate_audio_files(const ::tachyon::CompositionSpec& comp, const std::string& path, ValidationResult& out) const;

    // Estimate memory usage for the scene (in bytes)
    std::size_t estimate_memory(const ::tachyon::SceneSpec& scene) const;
};

} // namespace tachyon::core
