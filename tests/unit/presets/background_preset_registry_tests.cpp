#include <gtest/gtest.h>

#include "tachyon/presets/background/background_preset_registry.h"

#include <algorithm>
#include <string>
#include <vector>

using namespace tachyon::presets;
using tachyon::LayerType;

TEST(BackgroundPresetRegistryTests, ListsBuiltInPresets) {
    const auto presets = list_background_presets();

    EXPECT_NE(std::find(presets.begin(), presets.end(), "blank_canvas"), presets.end());
    EXPECT_NE(std::find(presets.begin(), presets.end(), "aurora_mesh"), presets.end());
    EXPECT_NE(std::find(presets.begin(), presets.end(), "liquid_glass_blobs"), presets.end());
    EXPECT_NE(std::find(presets.begin(), presets.end(), "dot_grid_fade"), presets.end());
    EXPECT_NE(std::find(presets.begin(), presets.end(), "noise_grain_vignette"), presets.end());
    EXPECT_NE(std::find(presets.begin(), presets.end(), "mesh_gradient"), presets.end());
    EXPECT_NE(std::find(presets.begin(), presets.end(), "mesh_gradient_from_output_shader"), presets.end());
    EXPECT_NE(std::find(presets.begin(), presets.end(), "dot_grid_fade_from_output_shader"), presets.end());
    EXPECT_NE(std::find(presets.begin(), presets.end(), "liquid_glass_blobs_from_output_cpp"), presets.end());
    EXPECT_NE(std::find(presets.begin(), presets.end(), "ripple_grid"), presets.end());
    EXPECT_NE(std::find(presets.begin(), presets.end(), "shape_grid_square"), presets.end());
}

TEST(BackgroundPresetRegistryTests, BuildsEachPreset) {
    const std::vector<std::string> ids = {
        "blank_canvas",
        "aurora_mesh",
        "liquid_glass_blobs",
        "dot_grid_fade",
        "noise_grain_vignette",
        "mesh_gradient",
        "mesh_gradient_from_output_shader",
        "dot_grid_fade_from_output_shader",
        "liquid_glass_blobs_from_output_cpp",
        "ripple_grid",
        "shape_grid_square"
    };

    for (const auto& id : ids) {
        auto bg = build_background_preset(id, 1920, 1080);
        ASSERT_TRUE(bg.has_value());

        if (id == "blank_canvas") {
            EXPECT_EQ(bg->type, "solid");
            EXPECT_EQ(bg->kind, LayerType::Solid);
        } else {
            EXPECT_EQ(bg->type, "procedural");
            EXPECT_EQ(bg->kind, LayerType::Procedural);
            ASSERT_TRUE(bg->procedural.has_value());
            EXPECT_FALSE(bg->procedural->kind.empty());
        }
    }
}
