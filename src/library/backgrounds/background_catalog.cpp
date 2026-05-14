#include "tachyon/presets/background/background_catalog.h"
#include "tachyon/core/spec/schema/objects/layer_spec.h"
#include "tachyon/presets/background/procedural.h"
#include <utility>

namespace tachyon {

namespace {

BackgroundDescriptor make_solid_background_descriptor() {
    BackgroundDescriptor desc;
    desc.id = "tachyon.background.solid";
    desc.aliases = {"solid", "color"};
    desc.kind = BackgroundKind::Solid;
    desc.params = registry::ParameterSchema{
        {"color", "Color", "Background color", ColorSpec(0, 0, 0)}
    };
    desc.capabilities.supports_cpu = true;
    desc.capabilities.supports_gpu = true;
    desc.display_name = "Solid Color";
    desc.description = "Solid color background";
    desc.build = [](const registry::ParameterBag& params) -> LayerSpec {
        LayerSpec spec;
        spec.identity.type = LayerType::Solid;
        spec.identity.name = "Solid Background";
        auto color = params.get_or<ColorSpec>("color", ColorSpec(0, 0, 0));
        spec.text.fill_color.keyframes = {{0.0, color}};
        return spec;
    };
    return desc;
}

BackgroundDescriptor make_gradient_background_descriptor() {
    BackgroundDescriptor desc;
    desc.id = "tachyon.background.gradient";
    desc.aliases = {"linear_gradient", "linear-gradient"};
    desc.kind = BackgroundKind::LinearGradient;
    desc.params = registry::ParameterSchema{
        {"color_start", "Start Color", "Gradient start color", ColorSpec(0, 0, 0)},
        {"color_end", "End Color", "Gradient end color", ColorSpec(255, 255, 255)},
        {"angle", "Angle", "Gradient angle in degrees", 0.0}
    };
    desc.capabilities.supports_cpu = true;
    desc.capabilities.supports_gpu = true;
    desc.display_name = "Linear Gradient";
    desc.description = "Linear gradient background";
    desc.build = [](const registry::ParameterBag& params) -> LayerSpec {
        LayerSpec spec;
        spec.identity.type = LayerType::Procedural;
        spec.identity.name = "Linear Gradient Background";

        ProceduralSpec proc;
        proc.kind = "tachyon.background.kind.linear_gradient";
        proc.color_a.keyframes = {{0.0, params.get_or<ColorSpec>("color_start", ColorSpec(0, 0, 0))}};
        proc.color_b.keyframes = {{0.0, params.get_or<ColorSpec>("color_end", ColorSpec(255, 255, 255))}};
        proc.angle.keyframes = {{0.0, params.get_or<double>("angle", 0.0)}};
        spec.source = ProceduralSource{ proc.kind, std::move(proc) };
        return spec;
    };
    return desc;
}

BackgroundDescriptor make_radial_gradient_background_descriptor() {
    BackgroundDescriptor desc;
    desc.id = "tachyon.background.radial_gradient";
    desc.aliases = {"radial_gradient", "radial-gradient"};
    desc.kind = BackgroundKind::RadialGradient;
    desc.params = registry::ParameterSchema{
        {"color_start", "Start Color", "Gradient start color", ColorSpec(0, 0, 0)},
        {"color_end", "End Color", "Gradient end color", ColorSpec(255, 255, 255)},
        {"center_x", "Center X", "Center X position (0-1)", 0.5},
        {"center_y", "Center Y", "Center Y position (0-1)", 0.5}
    };
    desc.capabilities.supports_cpu = true;
    desc.capabilities.supports_gpu = true;
    desc.display_name = "Radial Gradient";
    desc.description = "Radial gradient background";
    desc.build = [](const registry::ParameterBag& params) -> LayerSpec {
        LayerSpec spec;
        spec.identity.type = LayerType::Procedural;
        spec.identity.name = "Radial Gradient Background";

        ProceduralSpec proc;
        proc.kind = "tachyon.background.kind.radial_gradient";
        proc.color_a.keyframes = {{0.0, params.get_or<ColorSpec>("color_start", ColorSpec(0, 0, 0))}};
        proc.color_b.keyframes = {{0.0, params.get_or<ColorSpec>("color_end", ColorSpec(255, 255, 255))}};
        proc.focal_x = static_cast<float>(params.get_or<double>("center_x", 0.5));
        proc.focal_y = static_cast<float>(params.get_or<double>("center_y", 0.5));
        spec.source = ProceduralSource{ proc.kind, std::move(proc) };
        return spec;
    };
    return desc;
}

BackgroundDescriptor make_image_background_descriptor() {
    BackgroundDescriptor desc;
    desc.id = "tachyon.background.image";
    desc.aliases = {"image", "img"};
    desc.kind = BackgroundKind::Image;
    desc.params = registry::ParameterSchema{
        {"path", "Image Path", "Path to image file", std::string("")}
    };
    desc.capabilities.supports_cpu = true;
    desc.capabilities.supports_gpu = true;
    desc.display_name = "Image";
    desc.description = "Image background";
    desc.build = [](const registry::ParameterBag& params) -> LayerSpec {
        LayerSpec spec;
        spec.identity.type = LayerType::Image;
        spec.identity.name = "Image Background";
        spec.source = MediaSource{ params.get_or<std::string>("path", "") };
        return spec;
    };
    return desc;
}

BackgroundDescriptor make_video_background_descriptor() {
    BackgroundDescriptor desc;
    desc.id = "tachyon.background.video";
    desc.aliases = {"video", "vid"};
    desc.kind = BackgroundKind::Video;
    desc.params = registry::ParameterSchema{
        {"path", "Video Path", "Path to video file", std::string("")},
        {"loop", "Loop", "Whether to loop the video", true},
        {"mute", "Mute", "Whether to mute audio", true}
    };
    desc.capabilities.supports_cpu = true;
    desc.capabilities.supports_gpu = true;
    desc.display_name = "Video";
    desc.description = "Video background";
    desc.build = [](const registry::ParameterBag& params) -> LayerSpec {
        LayerSpec spec;
        spec.identity.type = LayerType::Video;
        spec.identity.name = "Video Background";
        spec.source = MediaSource{ params.get_or<std::string>("path", "") };
        return spec;
    };
    return desc;
}

} // namespace

namespace presets {

namespace {

using namespace tachyon::presets::background::procedural_bg;

BackgroundDescriptor make_procedural_background_descriptor(
    std::string id,
    std::string display_name,
    std::string description,
    BackgroundBuildFn build_fn) {
    BackgroundDescriptor desc;
    desc.id = std::move(id);
    desc.kind = BackgroundKind::Procedural;
    desc.display_name = std::move(display_name);
    desc.description = std::move(description);
    desc.build = std::move(build_fn);
    return desc;
}

std::vector<BackgroundDescriptor> build_builtin_background_descriptors() {
    std::vector<BackgroundDescriptor> descriptors;
    descriptors.reserve(12);

    descriptors.push_back(::tachyon::make_solid_background_descriptor());
    descriptors.push_back(::tachyon::make_gradient_background_descriptor());
    descriptors.push_back(::tachyon::make_radial_gradient_background_descriptor());
    descriptors.push_back(::tachyon::make_image_background_descriptor());
    descriptors.push_back(::tachyon::make_video_background_descriptor());

    auto get_procedural_params = [](const registry::ParameterBag& bag) {
        ProceduralParams p;
        p.palette_a = bag.get_or<ColorSpec>("palette.a", ColorSpec{0, 0, 0, 255});
        p.palette_b = bag.get_or<ColorSpec>("palette.b", ColorSpec{255, 255, 255, 255});
        p.palette_c = bag.get_or<ColorSpec>("palette.c", ColorSpec{0, 0, 0, 0});
        p.motion_speed = bag.get_or<double>("motion_speed", 1.0);
        p.contrast = bag.get_or<double>("contrast", 1.0);
        p.grain = bag.get_or<double>("grain", 0.0);
        p.softness = bag.get_or<double>("softness", 0.0);
        p.seed = static_cast<uint64_t>(bag.get_or<int>("seed", 0));
        return p;
    };

    auto make_procedural_builder = [get_procedural_params](auto make_spec_fn) {
        return [get_procedural_params, make_spec_fn](const registry::ParameterBag& bag) -> LayerSpec {
            LayerSpec layer;
            layer.identity.type = LayerType::Procedural;
            auto proc = make_spec_fn(get_procedural_params(bag));
            layer.source = ProceduralSource{ proc.kind, std::move(proc) };
            return layer;
        };
    };

    descriptors.push_back(make_procedural_background_descriptor(
        "tachyon.background.kind.aura",
        "Aura",
        "Soft aura background type",
        make_procedural_builder([](const ProceduralParams& p) { return make_aura_spec(p); })));

    descriptors.push_back(make_procedural_background_descriptor(
        "tachyon.background.kind.grid_modern",
        "Modern Grid",
        "Tech grid background type",
        make_procedural_builder([](const ProceduralParams& p) { return make_modern_tech_grid_spec(p); })));

    descriptors.push_back(make_procedural_background_descriptor(
        "tachyon.background.kind.stars",
        "Stars",
        "Star field background type",
        make_procedural_builder([](const ProceduralParams& p) { return make_stars_spec(p); })));

    descriptors.push_back(make_procedural_background_descriptor(
        "tachyon.background.kind.galaxy",
        "Galaxy",
        "Cinematic galaxy background type",
        make_procedural_builder([](const ProceduralParams& p) { return make_galaxy_spec(p); })));

    descriptors.push_back(make_procedural_background_descriptor(
        "tachyon.background.kind.nebula",
        "Nebula",
        "Cosmic nebula background type",
        make_procedural_builder([](const ProceduralParams& p) { return make_cosmic_nebula_spec(p); })));

    descriptors.push_back(make_procedural_background_descriptor(
        "tachyon.background.kind.silk",
        "Midnight Silk",
        "Silky cinematic background type",
        make_procedural_builder([](const ProceduralParams& p) { return make_midnight_silk_spec(p); })));

    descriptors.push_back(make_procedural_background_descriptor(
        "tachyon.background.kind.horizon",
        "Golden Horizon",
        "Warm horizon background type",
        make_procedural_builder([](const ProceduralParams& p) { return make_golden_horizon_spec(p); })));

    return descriptors;
}

} // namespace

const std::vector<BackgroundDescriptor>& builtin_background_descriptors() {
    static const std::vector<BackgroundDescriptor> descriptors = build_builtin_background_descriptors();
    return descriptors;
}

const BackgroundDescriptor* find_builtin_background_descriptor(std::string_view id) {
    const auto& descriptors = builtin_background_descriptors();
    for (const auto& descriptor : descriptors) {
        if (descriptor.id == id) {
            return &descriptor;
        }
    }
    return nullptr;
}

} // namespace presets
} // namespace tachyon
