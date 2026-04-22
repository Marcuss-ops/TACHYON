#pragma once

#include "tachyon/renderer2d/color/color_profile.h"
#include "tachyon/renderer2d/color/color_transform_graph.h"
#include <memory>
#include <string>

namespace tachyon::renderer2d {

class IColorManager {
public:
    virtual ~IColorManager() = default;

    // Load a configuration (e.g. OCIO config path)
    virtual bool load_config(const std::string& path) = 0;

    // Get a transform from one profile to another.
    // For professional pipelines, this should return a single optimized processor
    // (e.g., a ColorTransformGraph or an OCIO Processor).
    virtual std::shared_ptr<ColorTransformGraph> get_transform(const ColorProfile& src, const ColorProfile& dst) = 0;

    // Create a display/view transform (typical for UI preview).
    // This allows the renderer to process the working space surface into a display-ready buffer
    // in a single pass.
    virtual std::shared_ptr<ColorTransformGraph> get_display_transform(const ColorProfile& working, const std::string& display, const std::string& view) = 0;

    virtual std::string name() const = 0;
};

// Fallback implementation using the internal ColorTransformGraph logic
class InternalColorManager : public IColorManager {
public:
    bool load_config(const std::string&) override { return true; }

    std::shared_ptr<ColorTransformGraph> get_transform(const ColorProfile& src, const ColorProfile& dst) override {
        auto graph = std::make_shared<ColorTransformGraph>();
        graph->build_from_to(src, dst);
        return graph;
    }

    std::shared_ptr<ColorTransformGraph> get_display_transform(const ColorProfile& working, const std::string&, const std::string&) override {
        auto graph = std::make_shared<ColorTransformGraph>();
        graph->build_from_to(working, ColorProfile::sRGB());
        return graph;
    }

    std::string name() const override { return "Internal (Tachyon)"; }
};

#ifdef TACHYON_OCIO
class OCIOColorManager : public IColorManager {
public:
    bool load_config(const std::string& path) override;
    std::shared_ptr<ColorTransformGraph> get_transform(const ColorProfile& src, const ColorProfile& dst) override;
    std::shared_ptr<ColorTransformGraph> get_display_transform(const ColorProfile& working, const std::string& display, const std::string& view) override;
    std::string name() const override { return "OpenColorIO"; }
};
#endif

// Factory
std::unique_ptr<IColorManager> create_color_manager();

} // namespace tachyon::renderer2d
