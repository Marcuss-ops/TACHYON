#include "tachyon/renderer2d/evaluated_composition/matte_resolver.h"

// For backward compatibility, provide an alias
using MatteResolver = tachyon::renderer2d::MatteResolver;
#include "tachyon/core/scene/state/evaluated_state.h"
#include <cmath>
#include <iostream>
#include <string>

namespace {

int g_failures = 0;

void check_true(bool condition, const std::string& message) {
    if (!condition) {
        ++g_failures;
        std::cerr << "FAIL: " << message << '\n';
    }
}

bool nearly_equal(float a, float b, float eps = 1e-5f) {
    return std::abs(a - b) <= eps;
}

} // namespace

bool run_matte_resolver_tests() {
    g_failures = 0;

    using namespace tachyon::renderer2d;
    using namespace tachyon::scene;

    MatteResolver resolver;

    // Validation: empty layers and deps is valid
    {
        std::vector<EvaluatedLayerState> layers;
        std::vector<MatteDependency> deps;
        std::string error;
        bool ok = resolver.validate(layers, deps, error);
        check_true(ok, "empty validation passes");
    }

    // Validation: missing source ID fails
    {
        std::vector<EvaluatedLayerState> layers;
        EvaluatedLayerState layer;
        layer.id = "layer1";
        layers.push_back(layer);

        std::vector<MatteDependency> deps;
        deps.push_back({"missing_source", "layer1", MatteMode::Alpha});

        std::string error;
        bool ok = resolver.validate(layers, deps, error);
        check_true(!ok, "missing source id fails validation");
    }

    // Validation: missing target ID fails
    {
        std::vector<EvaluatedLayerState> layers;
        EvaluatedLayerState layer;
        layer.id = "layer1";
        layers.push_back(layer);

        std::vector<MatteDependency> deps;
        deps.push_back({"layer1", "missing_target", MatteMode::Alpha});

        std::string error;
        bool ok = resolver.validate(layers, deps, error);
        check_true(!ok, "missing target id fails validation");
    }

    // Validation: self-matting fails
    {
        std::vector<EvaluatedLayerState> layers;
        EvaluatedLayerState layer;
        layer.id = "layer1";
        layers.push_back(layer);

        std::vector<MatteDependency> deps;
        deps.push_back({"layer1", "layer1", MatteMode::Alpha});

        std::string error;
        bool ok = resolver.validate(layers, deps, error);
        check_true(!ok, "self-matting fails validation");
    }

    // Validation: cycle detection fails
    {
        std::vector<EvaluatedLayerState> layers;
        EvaluatedLayerState a, b, c;
        a.id = "A"; b.id = "B"; c.id = "C";
        layers = {a, b, c};

        std::vector<MatteDependency> deps;
        deps.push_back({"A", "B", MatteMode::Alpha});
        deps.push_back({"B", "C", MatteMode::Alpha});
        deps.push_back({"C", "A", MatteMode::Alpha});

        std::string error;
        bool ok = resolver.validate(layers, deps, error);
        check_true(!ok, "cycle detection fails validation");
    }

    // Validation: valid DAG passes
    {
        std::vector<EvaluatedLayerState> layers;
        EvaluatedLayerState a, b, c;
        a.id = "A"; b.id = "B"; c.id = "C";
        layers = {a, b, c};

        std::vector<MatteDependency> deps;
        deps.push_back({"A", "B", MatteMode::Alpha});
        deps.push_back({"A", "C", MatteMode::Luma});

        std::string error;
        bool ok = resolver.validate(layers, deps, error);
        check_true(ok, "valid DAG passes validation");
    }

    // Resolve: no dependencies -> all buffers are 1.0f
    {
        std::vector<EvaluatedLayerState> layers;
        EvaluatedLayerState layer;
        layer.id = "layer1";
        layers.push_back(layer);

        std::vector<MatteDependency> deps;
        std::vector<std::vector<float>> out_buffers;

        std::unordered_map<std::string, std::shared_ptr<SurfaceRGBA>> rendered;
        resolver.resolve(layers, deps, rendered, out_buffers, 4, 4);
        check_true(out_buffers.size() == 1, "resolve produces one buffer per layer");
        check_true(out_buffers[0].size() == 16, "buffer size = width*height");
        check_true(nearly_equal(out_buffers[0][0], 1.0f), "no-deps buffer is 1.0");
        check_true(nearly_equal(out_buffers[0][15], 1.0f), "no-deps buffer last is 1.0");
    }

    // Resolve: alpha matte copies source alpha
    {
        std::vector<EvaluatedLayerState> layers;
        EvaluatedLayerState src, tgt;
        src.id = "src"; src.active = true; src.visible = true;
        tgt.id = "tgt"; tgt.active = true; tgt.visible = true;
        layers = {src, tgt};

        std::vector<MatteDependency> deps;
        deps.push_back({"src", "tgt", MatteMode::Alpha});

        std::vector<std::vector<float>> out_buffers;
        std::unordered_map<std::string, std::shared_ptr<SurfaceRGBA>> rendered;
        resolver.resolve(layers, deps, rendered, out_buffers, 2, 2);
        check_true(out_buffers.size() == 2, "two buffers");
        // Buffer 0 (src) has no matte -> 1.0
        check_true(nearly_equal(out_buffers[0][0], 1.0f), "src buffer is 1.0");
        // Buffer 1 (tgt) uses src alpha (placeholder 0.5)
        check_true(nearly_equal(out_buffers[1][0], 0.5f), "alpha matte placeholder");
    }

    // Resolve: luma matte uses Rec.709 weights
    {
        std::vector<EvaluatedLayerState> layers;
        EvaluatedLayerState src, tgt;
        src.id = "src"; src.active = true; src.visible = true;
        tgt.id = "tgt"; tgt.active = true; tgt.visible = true;
        layers = {src, tgt};

        std::vector<MatteDependency> deps;
        deps.push_back({"src", "tgt", MatteMode::Luma});

        std::vector<std::vector<float>> out_buffers;
        std::unordered_map<std::string, std::shared_ptr<SurfaceRGBA>> rendered;
        resolver.resolve(layers, deps, rendered, out_buffers, 2, 2);
        check_true(out_buffers.size() == 2, "two buffers luma");
        check_true(nearly_equal(out_buffers[1][0], 0.5f), "luma matte placeholder");
    }

    // Resolve: invisible source produces zero matte
    {
        std::vector<EvaluatedLayerState> layers;
        EvaluatedLayerState src, tgt;
        src.id = "src"; src.active = false; src.visible = false;
        tgt.id = "tgt"; tgt.active = true; tgt.visible = true;
        layers = {src, tgt};

        std::vector<MatteDependency> deps;
        deps.push_back({"src", "tgt", MatteMode::Alpha});

        std::vector<std::vector<float>> out_buffers;
        std::unordered_map<std::string, std::shared_ptr<SurfaceRGBA>> rendered;
        resolver.resolve(layers, deps, rendered, out_buffers, 2, 2);
        check_true(nearly_equal(out_buffers[1][0], 0.0f), "invisible source -> zero matte");
    }

    return g_failures == 0;
}
