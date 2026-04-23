#include "tachyon/core/spec/compilation/scene_compiler.h"
#include "tachyon/runtime/cache/cache_key_builder.h"
#include "tachyon/runtime/memory/frame_arena.h"
#include "tachyon/runtime/core/data/property_graph.h"

#include <iostream>
#include <memory_resource>
#include <string>
#include <memory>

namespace {

int g_failures = 0;

void check_true(bool condition, const std::string& message) {
    if (!condition) {
        ++g_failures;
        std::cerr << "FAIL: " << message << '\n';
    }
}

} // namespace

bool run_runtime_backbone_tests() {
    using namespace tachyon;

    g_failures = 0;

    // Test CacheKeyBuilder
    auto builder_a = std::make_unique<CacheKeyBuilder>();
    builder_a->add_string("scene");
    builder_a->add_u64(42);

    auto builder_b = std::make_unique<CacheKeyBuilder>();
    builder_b->add_string("scene");
    builder_b->add_u64(42);

    auto builder_c = std::make_unique<CacheKeyBuilder>();
    builder_c->add_string("scene");
    builder_c->add_u64(43);

    check_true(builder_a->finish() == builder_b->finish(), "Deterministic cache key for identical input");
    check_true(builder_a->finish() != builder_c->finish(), "Different cache key for different input");

    // Test FrameArena
    auto arena = std::make_unique<FrameArena>();
    auto alloc = arena->allocator<int>();
    std::pmr::vector<int> values{alloc};
    values.push_back(1);
    values.push_back(2);
    check_true(values.size() == 2, "FrameArena allocates pmr vector data");
    arena->reset();
    values = std::pmr::vector<int>{arena->allocator<int>()};
    values.push_back(3);
    check_true(values.size() == 1, "FrameArena reset allows reuse");

    // Test SceneCompiler
    auto scene = std::make_unique<SceneSpec>();
    scene->version = "1.0";
    scene->spec_version = "1.0";
    scene->project.id = "project";
    scene->project.name = "project";

    CompositionSpec composition;
    composition.id = "main";
    composition.name = "main";
    composition.width = 1920;
    composition.height = 1080;
    composition.duration = 5.0;
    composition.frame_rate.numerator = 60;
    composition.frame_rate.denominator = 1;

    LayerSpec layer;
    layer.id = "layer-1";
    layer.name = "Layer 1";
    layer.type = "solid";
    layer.opacity = 0.8;
    composition.layers.push_back(layer);
    scene->compositions.push_back(composition);

    SceneCompiler compiler;
    const auto compiled_result = compiler.compile(*scene);
    check_true(compiled_result.ok(), "SceneCompiler compiles a minimal scene");
    if (compiled_result.ok()) {
        const auto& compiled = *compiled_result.value;
        check_true(compiled.compositions.size() == 1, "Compiled scene has one composition");
        // We now emit Opacity, PosX, PosY, ScaleX, ScaleY, Rotation, MaskFeather = 7 tracks
        check_true(compiled.property_tracks.size() == 7, "Compiled scene emits 7 transformation tracks");
        
        const auto& compiled_layer = compiled.compositions[0].layers[0];
        check_true(compiled_layer.property_indices.size() == 7, "Compiled layer has 7 property indices");
    }

    // Test PropertyGraph growth
    auto graph = std::make_unique<PropertyGraph>();
    try {
        std::uint32_t last_id = 0;
        for (int i = 0; i < 65; ++i) {
            last_id = graph->add_node({});
        }
        check_true(graph->nodes().size() == 65, "PropertyGraph grows past 64 nodes");
        check_true(last_id == 64, "PropertyGraph assigns monotonic node ids");
    } catch (const std::exception& e) {
        std::cerr << "Unexpected exception: " << e.what() << "\n";
        check_true(false, "PropertyGraph threw unexpectedly during growth test");
    }

    return g_failures == 0;
}
