#include "tachyon/renderer2d/effects/effect_registry.h"
#include "tachyon/renderer2d/effects/core/effect_host.h"
#include "tachyon/renderer2d/effects/core/glsl_transition_effect.h"
#include <utility>
#include <array>

namespace tachyon::renderer2d {

namespace {

static const std::array<EffectBuiltinSpec, 24> kBuiltinEffects = {{
    // --- Blur ---
    {
        "tachyon.effect.blur.gaussian", "Gaussian Blur", "blur",
        "Professional Gaussian blur with high-performance separation.", {"blur", "soften"},
        registry::ParameterSchema({{"blur_radius", "Radius", "Blur radius in pixels", 4.0, 0.0, 100.0}}),
        make_effect_factory<GaussianBlurEffect>()
    },
    {
        "tachyon.effect.blur.directional", "Directional Blur", "blur",
        "Motion-simulating directional blur.", {"blur", "direction"},
        registry::ParameterSchema({
            {"angle", "Angle", "Blur direction in degrees", 0.0, 0.0, 360.0},
            {"distance", "Distance", "Blur length in pixels", 10.0, 0.0, 500.0}
        }),
        make_effect_factory<DirectionalBlurEffect>()
    },
    {
        "tachyon.effect.blur.radial", "Radial Blur", "blur",
        "Circular blur around a center point.", {"blur", "radial"},
        registry::ParameterSchema({
            {"center_x", "Center X", "Radial center X (normalized)", 0.5, 0.0, 1.0},
            {"center_y", "Center Y", "Radial center Y (normalized)", 0.5, 0.0, 1.0},
            {"strength", "Strength", "Blur strength", 10.0, 0.0, 100.0}
        }),
        make_effect_factory<RadialBlurEffect>()
    },
    {
        "tachyon.effect.blur.vector", "Vector Blur", "blur",
        "Per-pixel motion-vector driven directional blur.", {"blur", "vector"},
        registry::ParameterSchema({
            {"angle", "Angle", "Blur angle in degrees", 0.0, 0.0, 360.0},
            {"distance", "Distance", "Blur distance in pixels", 10.0, 0.0, 500.0},
            {"samples", "Samples", "Number of samples", 8.0, 1.0, 64.0}
        }),
        make_effect_factory<VectorBlurEffect>()
    },

    // --- Color ---
    {
        "tachyon.effect.color.levels", "Levels", "color",
        "Advanced histogram-based color correction.", {"color", "histogram"},
        registry::ParameterSchema({
            {"input_black", "Input Black", "Input black point", 0.0, 0.0, 255.0},
            {"input_white", "Input White", "Input white point", 255.0, 0.0, 255.0},
            {"gamma", "Gamma", "Midtone adjustment", 1.0, 0.1, 10.0},
            {"output_black", "Output Black", "Output black point", 0.0, 0.0, 255.0},
            {"output_white", "Output White", "Output white point", 255.0, 0.0, 255.0}
        }),
        make_effect_factory<LevelsEffect>()
    },
    {
        "tachyon.effect.color.curves", "Curves", "color",
        "Precise color remapping using spline curves.", {"color", "curves"},
        registry::ParameterSchema({{"curve_points", "Curve Points", "JSON array of curve control points", "[]"}}),
        make_effect_factory<CurvesEffect>()
    },
    {
        "tachyon.effect.color.fill", "Fill", "color",
        "Solid color overlay with blending.", {"color", "overlay"},
        registry::ParameterSchema({
            {"color", "Color", "Fill color", ColorSpec{255, 255, 255, 255}},
            {"blend_mode", "Blend Mode", "How to blend fill", "normal"}
        }),
        make_effect_factory<FillEffect>()
    },
    {
        "tachyon.effect.color.tint", "Tint", "color",
        "Duo-tone color mapping.", {"color", "tint"},
        registry::ParameterSchema({
            {"amount", "Amount", "Tint intensity", 1.0, 0.0, 1.0},
            {"color", "Color", "Tint color", ColorSpec{255, 128, 0, 255}}
        }),
        make_effect_factory<TintEffect>()
    },
    {
        "tachyon.effect.color.hue_saturation", "Hue/Saturation", "color",
        "HSB adjustments for color vibrancy.", {"color", "hsb"},
        registry::ParameterSchema({
            {"hue_shift", "Hue Shift", "Hue rotation in degrees", 0.0, -180.0, 180.0},
            {"saturation", "Saturation", "Saturation adjustment", 0.0, -100.0, 100.0},
            {"lightness", "Lightness", "Lightness adjustment", 0.0, -100.0, 100.0}
        }),
        make_effect_factory<HueSaturationEffect>()
    },
    {
        "tachyon.effect.color.balance", "Color Balance", "color",
        "Individual RGB adjustments for shadows, midtones, and highlights.", {"color", "balance"},
        registry::ParameterSchema({
            {"shadows_r", "Shadows R", "Red in shadows", 0.0, -100.0, 100.0},
            {"shadows_g", "Shadows G", "Green in shadows", 0.0, -100.0, 100.0},
            {"shadows_b", "Shadows B", "Blue in shadows", 0.0, -100.0, 100.0},
            {"midtones_r", "Midtones R", "Red in midtones", 0.0, -100.0, 100.0},
            {"midtones_g", "Midtones G", "Green in midtones", 0.0, -100.0, 100.0},
            {"midtones_b", "Midtones B", "Blue in midtones", 0.0, -100.0, 100.0},
            {"highlights_r", "Highlights R", "Red in highlights", 0.0, -100.0, 100.0},
            {"highlights_g", "Highlights G", "Green in highlights", 0.0, -100.0, 100.0},
            {"highlights_b", "Highlights B", "Blue in highlights", 0.0, -100.0, 100.0}
        }),
        make_effect_factory<ColorBalanceEffect>()
    },
    {
        "tachyon.effect.color.lut", "LUT", "color",
        "Apply standard 2D Look-Up Tables.", {"color", "lut"},
        registry::ParameterSchema({
            {"lut_file", "LUT File", "Path to LUT file", ""},
            {"intensity", "Intensity", "LUT blend amount", 1.0, 0.0, 1.0}
        }),
        make_effect_factory<LUTEffect>()
    },
    {
        "tachyon.effect.color.lut3d", "3D LUT (.cube)", "color",
        "Professional 3D LUT application.", {"color", "lut3d"},
        registry::ParameterSchema({
            {"lut_file", "LUT File", "Path to .cube file", ""},
            {"intensity", "Intensity", "LUT blend amount", 1.0, 0.0, 1.0}
        }),
        make_effect_factory<Lut3DCubeEffect>()
    },

    // --- Distort ---
    {
        "tachyon.effect.distort.vignette", "Vignette", "distort",
        "Soft darkening of the image edges.", {"distort", "vignette"},
        registry::ParameterSchema({
            {"radius", "Radius", "Vignette size", 0.5, 0.0, 1.0},
            {"softness", "Softness", "Edge feathering", 0.5, 0.0, 1.0},
            {"strength", "Strength", "Darkening amount", 1.0, 0.0, 1.0}
        }),
        make_effect_factory<VignetteEffect>()
    },
    {
        "tachyon.effect.distort.chromatic_aberration", "Chromatic Aberration", "distort",
        "Simulates optical lens color fringing.", {"distort", "lens"},
        registry::ParameterSchema({
            {"offset_r", "Offset R", "Red channel shift", 2.0},
            {"offset_b", "Offset B", "Blue channel shift", -2.0}
        }),
        make_effect_factory<ChromaticAberrationEffect>()
    },
    {
        "tachyon.effect.distort.displacement", "Displacement Map", "distort",
        "Geometric distortion using another layer's luminance.", {"distort", "displacement"},
        registry::ParameterSchema({
            {"map_layer", "Map Layer", "Layer ID for displacement", ""},
            {"strength_x", "Strength X", "Horizontal displacement", 10.0, -500.0, 500.0},
            {"strength_y", "Strength Y", "Vertical displacement", 10.0, -500.0, 500.0}
        }),
        make_effect_factory<DisplacementMapEffect>()
    },
    {
        "tachyon.effect.distort.warp_stabilizer", "Warp Stabilizer", "distort",
        "Warp-based image stabilization.", {"distort", "stabilize"},
        registry::ParameterSchema({
            {"crop_percent", "Crop %", "Auto-crop amount", 10.0, 0.0, 100.0},
            {"smoothing", "Smoothing", "Stabilization strength", 50.0, 0.0, 100.0}
        }),
        make_effect_factory<WarpStabilizerEffect>()
    },

    // --- Generator ---
    {
        "tachyon.effect.generator.light_leak", "Light Leak", "generator",
        "Procedural light leak and film burn effect.", {"light", "film", "vintage"},
        registry::ParameterSchema({
            {"progress", "Progress", "Animation progress (0-1)", 0.0, 0.0, 1.0},
            {"speed", "Speed", "Animation speed multiplier", 1.0, 0.1, 10.0},
            {"seed", "Seed", "Random seed", 3.0},
            {"preset", "Preset", "Preset type (0-N)", 0.0},
            {"intensity", "Intensity", "Light intensity", 1.0, 0.0, 5.0},
            {"width", "Width", "Leak width", 0.5, 0.0, 2.0},
            {"color_a", "Color A", "Start color", ColorSpec{255, 204, 153, 255}},
            {"color_b", "Color B", "End color", ColorSpec{255, 102, 51, 255}}
        }),
        make_effect_factory<LightLeakEffect>()
    },
    {
        "tachyon.effect.generator.particle_emitter", "Particle Emitter", "generator",
        "Simple procedural particle system.", {"particle", "emitter", "physics"},
        registry::ParameterSchema({
            {"time", "Time", "Current time in seconds", 0.0},
            {"seed", "Seed", "Random seed", 1.0},
            {"count", "Count", "Number of particles", 48.0, 1.0, 500.0},
            {"lifetime", "Lifetime", "Particle lifetime in seconds", 1.0, 0.1, 10.0},
            {"speed", "Speed", "Initial speed", 18.0},
            {"gravity", "Gravity", "Gravity force (Y-axis)", 0.0},
            {"spread_x", "Spread X", "Emission spread X", 1.0},
            {"spread_y", "Spread Y", "Emission spread Y", 1.0},
            {"radius_min", "Min Radius", "Minimum particle size", 1.0},
            {"radius_max", "Max Radius", "Maximum particle size", 3.0},
            {"color", "Color", "Particle color", ColorSpec{255, 255, 255, 166}},
            {"opacity", "Opacity", "Global opacity", 1.0, 0.0, 1.0},
            {"center_x", "Center X", "Emitter X position (normalized)", 0.5},
            {"center_y", "Center Y", "Emitter Y position (normalized)", 0.5},
            {"emit_width", "Emit Width", "Width of the emission area", 1920.0},
            {"emit_height", "Emit Height", "Height of the emission area", 1080.0}
        }),
        make_effect_factory<ParticleEmitterEffect>()
    },
    {
        "tachyon.effect.generator.lens_flare", "Lens Flare", "generator",
        "Simulated optical lens flare.", {"lens", "flare", "optical"},
        registry::ParameterSchema({{"lights", "Lights", "Encoded light configuration string", ""}}),
        make_effect_factory<LensFlareEffect>()
    },

    // --- Stylize & Keying ---
    {
        "tachyon.effect.stylize.drop_shadow", "Drop Shadow", "stylize",
        "Adds a soft shadow behind the layer.", {"shadow", "drop", "depth"},
        registry::ParameterSchema({
            {"blur_radius", "Blur Radius", "Shadow softness", 4.0, 0.0, 100.0},
            {"offset_x", "Offset X", "Horizontal shift", 4.0},
            {"offset_y", "Offset Y", "Vertical shift", 4.0},
            {"shadow_color", "Color", "Shadow color", ColorSpec{0, 0, 0, 160}}
        }),
        make_effect_factory<DropShadowEffect>()
    },
    {
        "tachyon.effect.stylize.glow", "Glow", "stylize",
        "Adds a luminous halo to bright areas.", {"glow", "bloom", "light"},
        registry::ParameterSchema({
            {"radius", "Radius", "Glow spread", 4.0, 0.0, 100.0},
            {"strength", "Strength", "Glow intensity", 1.0, 0.0, 10.0},
            {"threshold", "Threshold", "Luminance threshold", 0.0, 0.0, 1.0}
        }),
        make_effect_factory<GlowEffect>()
    },
    {
        "tachyon.effect.stylize.motion_blur_2d", "Motion Blur 2D", "stylize",
        "Linear directional blur for motion simulation.", {"motion", "blur", "speed"},
        registry::ParameterSchema({
            {"samples", "Samples", "Number of blur samples", 8.0, 1.0, 64.0},
            {"angle", "Angle", "Blur direction in degrees", 0.0, 0.0, 360.0},
            {"distance", "Distance", "Blur length in pixels", 10.0, 0.0, 500.0},
            {"shutter_angle", "Shutter Angle", "Simulated shutter (180 = half frame)", 180.0, 0.0, 720.0}
        }),
        make_effect_factory<MotionBlur2DEffect>()
    },
    {
        "tachyon.effect.keying.chroma_key", "Chroma Key", "keying",
        "Removes a specific color from the image.", {"keying", "chroma", "green-screen"},
        registry::ParameterSchema({
            {"key_color", "Key Color", "Color to remove", ColorSpec{0, 255, 0, 255}},
            {"threshold", "Threshold", "Sensitivity", 0.1, 0.0, 1.0},
            {"softness", "Softness", "Edge softness", 0.1, 0.0, 1.0}
        }),
        make_effect_factory<ChromaKeyEffect>()
    },

    // --- Custom ---
    {
        "tachyon.effect.distort.glitch", "Digital Glitch", "distort",
        "Digital artifacting and displacement.", {"glitch", "artifact"},
        registry::ParameterSchema({
            {"intensity", "Intensity", "Glitch strength", 0.5, 0.0, 1.0},
            {"seed", "Seed", "Random seed", 0.0, 0.0, 9999.0}
        }),
        [](const EffectSpec&, const SurfaceRGBA& input, SurfaceRGBA& output,
           const std::vector<const SurfaceRGBA*>&, const EffectParams&) {
            output = input; // Placeholder implementation
        }
    }
}};

} // namespace

EffectRegistry::EffectRegistry() {
}

void EffectRegistry::register_spec(EffectDescriptor descriptor) {
    if (descriptor.id.empty()) {
        return;
    }
    registry_.register_spec(std::move(descriptor));
}

const EffectDescriptor* EffectRegistry::find(std::string_view id) const {
    return registry_.find(id);
}

std::vector<std::string> EffectRegistry::list_ids() const {
    return registry_.list_ids();
}

void register_builtin_effects(EffectRegistry& registry, const tachyon::TransitionRegistry& transition_registry) {
    // 1. Register most effects from the static array
    for (const auto& spec : kBuiltinEffects) {
        register_effect_from_spec(registry, spec);
    }

    // 2. Register GLSL Transition effect (requires transition_registry)
    EffectBuiltinSpec transition_spec = {
        "tachyon.effect.transition.glsl", "GLSL Transition", "transition",
        "GPU-accelerated GLSL transition kernel.", {"glsl", "transition", "gpu"},
        registry::ParameterSchema({
            {"progress", "Progress", "Transition progress", 0.0, 0.0, 1.0},
            {"transition_id", "Transition", "ID of the GLSL transition", "fade"}
        }),
        nullptr, // Factory set manually below
        {{"transition_to", "transition_to_layer_id", "transition", false}},
        true,
        false
    };

    // Manual factory to inject transition_registry
    transition_spec.factory = [&transition_registry](const EffectSpec&, const SurfaceRGBA& input, SurfaceRGBA& output,
                                                   const std::vector<const SurfaceRGBA*>&, const EffectParams& params) {
        GlslTransitionEffect effect(transition_registry);
        output = effect.apply(input, params);
    };

    register_effect_from_spec(registry, transition_spec);
}

void register_effect_from_spec(EffectRegistry& registry, const EffectBuiltinSpec& spec) {
    EffectDescriptor descriptor;
    descriptor.id = std::string(spec.id);
    descriptor.metadata.display_name = std::string(spec.display_name);
    descriptor.metadata.category = "effect." + std::string(spec.category);
    descriptor.metadata.description = std::string(spec.description);
    
    for (auto tag : spec.tags) {
        descriptor.metadata.tags.push_back(std::string(tag));
    }
    
    descriptor.schema = spec.schema;
    descriptor.factory = spec.factory;
    descriptor.aux_requirements = spec.aux_requirements;
    descriptor.is_deterministic = spec.is_deterministic;
    descriptor.supports_gpu_acceleration = spec.supports_gpu_acceleration;
    
    registry.register_spec(std::move(descriptor));
}

} // namespace tachyon::renderer2d
