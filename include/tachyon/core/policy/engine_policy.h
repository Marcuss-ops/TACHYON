#pragma once

namespace tachyon {

/**
 * @brief Global policy for engine validation and fallback behaviors.
 * 
 * In development, loose validation is preferred for iteration speed.
 * In production, strict validation is required to prevent visual regression or silent failures.
 */
struct EngineValidationPolicy {
    bool strict_transitions{true};    ///< Error if transition ID is missing
    bool strict_backgrounds{true};     ///< Error if background preset or color is invalid
    bool strict_assets{true};          ///< Error if assets (images, fonts) are missing
    bool strict_text_layout{true};     ///< Error if text overflow or font missing
    bool strict_3d_modifiers{true};    ///< Error if 3D modifier registry is unaligned

    static EngineValidationPolicy& instance() {
        static EngineValidationPolicy policy;
        return policy;
    }
    
    /**
     * @brief Set global strictness for all subsystems.
     */
    void set_all_strict(bool strict) {
        strict_transitions = strict;
        strict_backgrounds = strict;
        strict_assets = strict;
        strict_text_layout = strict;
        strict_3d_modifiers = strict;
    }
};

} // namespace tachyon
