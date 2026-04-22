#include "tachyon/renderer2d/color/color_management_system.h"

namespace tachyon::renderer2d {

ColorManagementSystem::ColorManagementSystem() {
    m_manager = create_color_manager();
}

ColorTransformGraph ColorManagementSystem::build_input_to_working() const {
    auto g = m_manager->get_transform(input_profile, working_profile);
    return *g;
}

ColorTransformGraph ColorManagementSystem::build_working_to_output() const {
    auto g = m_manager->get_transform(working_profile, output_profile);
    return *g;
}

ColorTransformGraph ColorManagementSystem::build_working_to_view() const {
    auto g = m_manager->get_transform(working_profile, view_profile);
    return *g;
}

bool ColorManagementSystem::load_ocio_config(const std::string& config_path) {
    return m_manager->load_config(config_path);
}

bool ColorManagementSystem::uses_ocio() const {
    return m_manager->name() == "OpenColorIO";
}

ColorManagementSystem ColorManagementSystem::default_pipeline() {
    ColorManagementSystem cms;
    cms.input_profile   = ColorProfile::sRGB();
    cms.working_profile = ColorProfile::ACEScg();
    cms.output_profile  = ColorProfile::Rec709();
    cms.view_profile    = ColorProfile::sRGB();
    return cms;
}

ColorManagementSystem ColorManagementSystem::srgb_passthrough() {
    ColorManagementSystem cms;
    cms.input_profile   = ColorProfile::sRGB();
    cms.working_profile = ColorProfile::sRGB();
    cms.output_profile  = ColorProfile::sRGB();
    cms.view_profile    = ColorProfile::sRGB();
    return cms;
}

} // namespace tachyon::renderer2d
