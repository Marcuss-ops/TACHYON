#pragma once

#include <vector>

namespace tachyon::color {

struct WaveformColumn {
    std::vector<float> luminance;
};

struct WaveformResult {
    std::vector<WaveformColumn> columns; // One per X pixel
};

struct VectorscopeResult {
    // 2D histogram of Cb/Cr or u/v
    std::vector<std::vector<float>> grid;
};

struct HistogramResult {
    std::vector<int> r_bins;
    std::vector<int> g_bins;
    std::vector<int> b_bins;
    std::vector<int> luma_bins;
};

/**
 * @brief Analysis pass over a rendered frame to generate scopes data.
 */
class ScopesAnalyzer {
public:
    // In a real implementation these would take a SurfaceRGBA or similar buffer.
    
    static WaveformResult analyze_waveform(/* const SurfaceRGBA& frame */) {
        return WaveformResult{};
    }

    static VectorscopeResult analyze_vectorscope(/* const SurfaceRGBA& frame */) {
        return VectorscopeResult{};
    }

    static HistogramResult analyze_histogram(/* const SurfaceRGBA& frame, int bins = 256 */) {
        return HistogramResult{};
    }
};

} // namespace tachyon::color
