#include "tachyon/core/expressions/context_builder.h"
#include <algorithm>
#include <cmath>
#include <cstdlib>

namespace tachyon::expressions {

namespace {

constexpr double kDefaultTableValue = 0.0;

double parse_table_value(const std::string& value) {
    const char* begin = value.c_str();
    char* end = nullptr;
    const double parsed = std::strtod(begin, &end);
    if (end == begin || *end != '\0') {
        return kDefaultTableValue;
    }
    return parsed;
}

} // namespace

ExpressionContext build_context(const ExpressionContextInput& input) {
    ExpressionContext ctx;
    ctx.layer_index = input.layer_index;
    ctx.property_sampler = input.property_sampler;
    ctx.time = input.time;
    ctx.value = input.fallback_value;
    ctx.seed = input.seed;

    // Standard variables
    ctx.variables["t"] = input.time;
    ctx.variables["time"] = input.time;
    ctx.variables["seed"] = static_cast<double>(input.seed);

    // Job variables
    if (input.job_variables) {
        for (const auto& [k, v] : *input.job_variables) {
            ctx.variables.try_emplace(k, v);
        }
    }

    // Tables and Table Lookup
    if (input.tables) {
        ctx.tables = *input.tables;
        
        // Pre-sort table keys for consistent indexing
        std::vector<std::string> table_keys;
        table_keys.reserve(input.tables->size());
        for (const auto& [key, _] : *input.tables) {
            table_keys.push_back(key);
        }
        std::sort(table_keys.begin(), table_keys.end());

        ctx.table_lookup = [tables = input.tables, keys = std::move(table_keys)](double table_idx, double row, double col) -> double {
            if (!tables || tables->empty() || keys.empty()) {
                return 0.0;
            }
            const auto idx = static_cast<std::size_t>(std::max(0.0, std::floor(table_idx)));
            if (idx >= keys.size()) {
                return 0.0;
            }
            const auto& table = tables->at(keys[idx]);
            const std::size_t r = static_cast<std::size_t>(std::max(0.0, std::floor(row)));
            const std::size_t c = static_cast<std::size_t>(std::max(0.0, std::floor(col)));
            if (r >= table.size() || c >= table[r].size()) {
                return 0.0;
            }
            return parse_table_value(table[r][c]);
        };
    }

    // Audio Analysis
    if (input.audio_analyzer) {
        const audio::AudioBands bands = input.audio_analyzer->analyze_frame(input.time);
        
        // Legacy variable access
        ctx.variables["music.bass"] = bands.bass;
        ctx.variables["music.mid"] = bands.mid;
        ctx.variables["music.high"] = bands.high;
        ctx.variables["music.presence"] = bands.presence;
        ctx.variables["music.rms"] = bands.rms;

        // Structured data access
        ctx.audio_analysis.bass = bands.bass;
        ctx.audio_analysis.mid = bands.mid;
        ctx.audio_analysis.treble = bands.high; // Map high to treble
        ctx.audio_analysis.rms = bands.rms;
        // beat is not yet implemented in AudioBands
    }

    // Property Boundaries
    if (input.prop_start.has_value() && input.prop_end.has_value()) {
        ctx.variables["_prop_start"] = *input.prop_start;
        ctx.variables["_prop_end"] = *input.prop_end;
        ctx.variables["_prop_duration"] = *input.prop_end - *input.prop_start;
    }

    return ctx;
}

} // namespace tachyon::expressions
