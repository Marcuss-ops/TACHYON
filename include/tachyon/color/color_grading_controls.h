#pragma once

#include <vector>

namespace tachyon::color {

struct PrimaryGrading {
    double lift[3]{0.0, 0.0, 0.0};
    double gamma[3]{1.0, 1.0, 1.0};
    double gain[3]{1.0, 1.0, 1.0};
    double offset[3]{0.0, 0.0, 0.0};

    double exposure{0.0};
    double contrast{1.0};
    double saturation{1.0};
    double temperature{0.0};
    double tint{0.0};
};

struct CurvePoint {
    double x;
    double y;
};

struct SecondaryGrading {
    std::vector<CurvePoint> rgb_curve_r;
    std::vector<CurvePoint> rgb_curve_g;
    std::vector<CurvePoint> rgb_curve_b;
    std::vector<CurvePoint> rgb_curve_master;

    std::vector<CurvePoint> hue_vs_hue;
    std::vector<CurvePoint> hue_vs_sat;
    std::vector<CurvePoint> hue_vs_lum;
};

/**
 * @brief Represents an active color grading effect in the pipeline.
 * Does not manipulate pixels directly, but generates an OCIO-compatible Transform
 * or custom shader parameters for the GPU backend.
 */
class ColorGradingNode {
public:
    ColorGradingNode() = default;

    void set_primary(const PrimaryGrading& primary) { m_primary = primary; }
    void set_secondary(const SecondaryGrading& secondary) { m_secondary = secondary; }

    /**
     * @brief Builds a 3D LUT or parameters array for the render engine.
     */
    void build_transform_pipeline() {
        // Implementation stub: 
        // 1. Map exposure/contrast to a CDL (Color Decision List)
        // 2. Map Lift/Gamma/Gain/Offset to ASC CDL
        // 3. Compile curves into a 1D or 3D LUT
    }

private:
    PrimaryGrading m_primary;
    SecondaryGrading m_secondary;
};

} // namespace tachyon::color
