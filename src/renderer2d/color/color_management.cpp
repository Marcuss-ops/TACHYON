#include "tachyon/renderer2d/color/color_management.h"

namespace tachyon::renderer2d {

#ifdef TACHYON_OCIO
// This would link against OCIO SDK
// #include <OpenColorIO/OpenColorIO.h>
// namespace OCIO = OCIO_NAMESPACE;

bool OCIOColorManager::load_config(const std::string& path) {
    // try {
    //     m_config = OCIO::Config::CreateFromFile(path.c_str());
    //     return true;
    // } catch (...) { return false; }
    return false; // Stub
}

std::shared_ptr<ColorTransformGraph> OCIOColorManager::get_transform(const ColorProfile& src, const ColorProfile& dst) {
    // Real implementation would create an OCIO processor and wrap it
    // into a ColorTransformGraph step.
    auto graph = std::make_shared<ColorTransformGraph>();
    graph->build_from_to(src, dst);
    return graph;
}

std::shared_ptr<ColorTransformGraph> OCIOColorManager::get_display_transform(const ColorProfile& working, const std::string& display, const std::string& view) {
    // Create an OCIO DisplayViewTransform
    auto graph = std::make_shared<ColorTransformGraph>();
    graph->build_from_to(working, ColorProfile::sRGB());
    return graph;
}
#endif

std::unique_ptr<IColorManager> create_color_manager() {
#ifdef TACHYON_OCIO
    return std::make_unique<OCIOColorManager>();
#else
    return std::make_unique<InternalColorManager>();
#endif
}

} // namespace tachyon::renderer2d
