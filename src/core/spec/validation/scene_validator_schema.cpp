#include "tachyon/core/spec/validation/scene_validator.h"

namespace tachyon::core {

void SceneValidator::validate_schema_version(const ::tachyon::SceneSpec& scene, ValidationResult& out) const {
    // Check if schema version is present and valid
    if (scene.schema_version.major == 0) {
        out.issues.push_back(ValidationIssue{ValidationIssue::Severity::Warning, "scene.schema_version", 
            "Schema version is 0.0.0. This may indicate an older file format."});
        out.warning_count++;
    }
    
    // Future: Add version compatibility checks here
    // For now, we just warn about very old versions
    static const SchemaVersion min_supported{0, 9, 0};
    if (scene.schema_version < min_supported) {
        out.issues.push_back(ValidationIssue{ValidationIssue::Severity::Error, "scene.schema_version",
            "Schema version " + scene.schema_version.to_string() + " is not supported. Minimum required: " + min_supported.to_string()});
        out.error_count++;
    }
}

} // namespace tachyon::core
