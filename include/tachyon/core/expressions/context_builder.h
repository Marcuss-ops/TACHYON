#pragma once

#include "tachyon/core/expressions/expression_engine.h"
#include "tachyon/audio/audio_analyzer.h"
#include <string>
#include <unordered_map>
#include <vector>
#include <optional>

namespace tachyon::expressions {

/**
 * @brief Input parameters for building an expression evaluation context.
 */
struct ExpressionContextInput {
    double time{0.0};
    std::uint64_t seed{0};
    std::uint32_t layer_index{0};
    double fallback_value{0.0};
    
    const audio::AudioAnalyzer* audio_analyzer{nullptr};
    const std::unordered_map<std::string, double>* job_variables{nullptr};
    const std::unordered_map<std::string, std::vector<std::vector<std::string>>>* tables{nullptr};
    
    // Callback for cross-property sampling (valueAtTime)
    std::function<double(int, double)> property_sampler;
    
    // Property-specific boundaries (optional)
    std::optional<double> prop_start;
    std::optional<double> prop_end;
};

/**
 * @brief Builds a standardized ExpressionContext from raw inputs.
 * This ensures consistent variable availability (audio, tables, time, etc.)
 * across all property types (Scalar, Vector, Color).
 */
ExpressionContext build_context(const ExpressionContextInput& input);

} // namespace tachyon::expressions
