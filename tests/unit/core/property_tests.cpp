/**
 * property_tests.cpp — Golden tests for the TACHYON Property System.
 *
 * Test categories:
 *  1. ConstantProperty (static value model)
 *  2. AnimationCurve — linear interpolation, golden table
 *  3. AnimationCurve — Hold mode
 *  4. AnimationCurve — Easing (EaseIn, EaseOut, EaseInOut)
 *  5. AnimationCurve — Bezier Hermite tangents
 *  6. AnimatableProperty — static mode, version invalidation
 *  7. AnimatableProperty — animated mode, sampling, hash identity
 *  8. AnimatableProperty — Vector3 typed curve
 *  9. PropertyGroup — declare, sample, factory helpers
 * 10. Boundary conditions: empty curve, single keyframe, out-of-range t
 */

#include "tachyon/core/properties/property.h"
#include "tachyon/core/animation/animatable.h"
#include "tachyon/core/math/vector3.h"
#include "tachyon/core/math/vector2.h"
#include "tachyon/properties/animatable_property.h"
#include "tachyon/properties/property_group.h"

#include <cmath>
#include <iostream>
#include <string>

namespace {

// ---------------------------------------------------------------------------
// Minimal test harness (no external framework dependency)
// ---------------------------------------------------------------------------

int g_failures = 0;

void check_true(bool condition, const std::string& msg) {
    if (!condition) {
        ++g_failures;
        std::cerr << "  FAIL: " << msg << '\n';
    }
}

bool nearly_eq(float a, float b, float eps = 1e-5f) {
    return std::abs(a - b) <= eps;
}

bool nearly_eq(double a, double b, double eps = 1e-8) {
    return std::abs(a - b) <= eps;
}

bool nearly_eq_v3(const tachyon::math::Vector3& a,
                  const tachyon::math::Vector3& b,
                  float eps = 1e-5f) {
    return nearly_eq(a.x, b.x, eps) &&
           nearly_eq(a.y, b.y, eps) &&
           nearly_eq(a.z, b.z, eps);
}

// ---------------------------------------------------------------------------
// Section 1: ConstantProperty
// ---------------------------------------------------------------------------
void test_constant_property() {
    using namespace tachyon::properties;

    ConstantProperty<float> opacity("Opacity", 0.75f);
    PropertyEvaluationContext ctx{ 5.0, 0 };

    check_true(nearly_eq(opacity.sample(ctx), 0.75f),
               "S1: ConstantProperty<float> sample returns static value");
    check_true(opacity.get_name() == "Opacity",
               "S1: ConstantProperty name metadata");

    // Same hash at any time
    PropertyEvaluationContext ctx2{ 99.0, 42 };
    check_true(opacity.hash_identity(ctx) == opacity.hash_identity(ctx2),
               "S1: ConstantProperty hash is time-invariant");
}

// ---------------------------------------------------------------------------
// Section 2: AnimationCurve — linear interpolation golden table
// ---------------------------------------------------------------------------
void test_curve_linear_golden() {
    using namespace tachyon::animation;

    AnimationCurve<float> curve;
    // Two keyframes: 0s → 10, 1s → 20
    curve.keyframes = {
        { 0.0, 10.0f, InterpolationMode::Linear, InterpolationMode::Linear, EasingPreset::None },
        { 1.0, 20.0f, InterpolationMode::Linear, InterpolationMode::Linear, EasingPreset::None }
    };

    // Golden table (time → expected value)
    const struct { double t; float expected; } golden[] = {
        {0.0,  10.0f},
        {0.25, 12.5f},
        {0.5,  15.0f},
        {0.75, 17.5f},
        {1.0,  20.0f},
    };

    for (const auto& g : golden) {
        const float v = curve.evaluate(g.t);
        check_true(nearly_eq(v, g.expected),
                   "S2: linear golden t=" + std::to_string(g.t) +
                   " expected=" + std::to_string(g.expected) +
                   " got=" + std::to_string(v));
    }
}

// ---------------------------------------------------------------------------
// Section 3: AnimationCurve — Hold mode
// ---------------------------------------------------------------------------
void test_curve_hold() {
    using namespace tachyon::animation;

    AnimationCurve<float> curve;
    curve.keyframes = {
        { 0.0, 5.0f,  InterpolationMode::Hold, InterpolationMode::Hold, EasingPreset::None },
        { 1.0, 15.0f, InterpolationMode::Hold, InterpolationMode::Hold, EasingPreset::None },
        { 2.0, 30.0f, InterpolationMode::Hold, InterpolationMode::Hold, EasingPreset::None },
    };

    check_true(nearly_eq(curve.evaluate(0.0),  5.0f),  "S3: hold at t=0 → 5");
    check_true(nearly_eq(curve.evaluate(0.99), 5.0f),  "S3: hold at t=0.99 → 5 (before jump)");
    check_true(nearly_eq(curve.evaluate(1.0),  15.0f), "S3: hold at t=1.0 → 15 (at boundary)");
    check_true(nearly_eq(curve.evaluate(1.5),  15.0f), "S3: hold at t=1.5 → 15");
    check_true(nearly_eq(curve.evaluate(2.0),  30.0f), "S3: hold at t=2.0 → 30");
    check_true(nearly_eq(curve.evaluate(5.0),  30.0f), "S3: hold past end clamps to last");
}

// ---------------------------------------------------------------------------
// Section 4: AnimationCurve — Easing presets
// Golden values derived analytically from the CSS cubic-bezier spec.
// ---------------------------------------------------------------------------
void test_curve_easing() {
    using namespace tachyon::animation;

    // Curve from 0 to 100 over 1 second, eased
    auto make_eased_curve = [](EasingPreset preset) {
        AnimationCurve<float> c;
        Keyframe<float> k0, k1;
        k0.time = 0.0; k0.value = 0.0f;
        k0.out_mode = InterpolationMode::Linear;
        k0.easing   = preset;
        k1.time = 1.0; k1.value = 100.0f;
        k1.out_mode = InterpolationMode::Linear;
        k1.easing   = EasingPreset::None;
        c.keyframes = {k0, k1};
        return c;
    };

    // EaseIn: starts slow → at t=0.5, value should be < 50 (biased toward end)
    {
        auto c = make_eased_curve(EasingPreset::EaseIn);
        const float vmid = c.evaluate(0.5);
        check_true(vmid < 50.0f, "S4: EaseIn at t=0.5 should be < 50 (slow start)");
        check_true(nearly_eq(c.evaluate(0.0), 0.0f, 1e-3f), "S4: EaseIn t=0 → 0");
        check_true(nearly_eq(c.evaluate(1.0), 100.0f, 1e-3f), "S4: EaseIn t=1 → 100");
    }

    // EaseOut: fast start → at t=0.5, value should be > 50
    {
        auto c = make_eased_curve(EasingPreset::EaseOut);
        const float vmid = c.evaluate(0.5);
        check_true(vmid > 50.0f, "S4: EaseOut at t=0.5 should be > 50 (fast start)");
        check_true(nearly_eq(c.evaluate(0.0), 0.0f, 1e-3f), "S4: EaseOut t=0 → 0");
        check_true(nearly_eq(c.evaluate(1.0), 100.0f, 1e-3f), "S4: EaseOut t=1 → 100");
    }

    // EaseInOut: symmetric → at t=0.5, value should be exactly 50 ± eps
    {
        auto c = make_eased_curve(EasingPreset::EaseInOut);
        const float vmid = c.evaluate(0.5);
        check_true(nearly_eq(vmid, 50.0f, 0.2f), "S4: EaseInOut at t=0.5 ≈ 50 (symmetric)");
    }
}

// ---------------------------------------------------------------------------
// Section 5: AnimationCurve — Bezier Hermite tangents
// ---------------------------------------------------------------------------
void test_curve_bezier_hermite() {
    using namespace tachyon::animation;

    AnimationCurve<float> curve;
    Keyframe<float> k0, k1;

    k0.time = 0.0; k0.value = 0.0f;
    k0.out_mode           = InterpolationMode::Bezier;
    k0.easing             = EasingPreset::None;
    k0.out_tangent_time   = 0.333;
    k0.out_tangent_value  = 50.0f;  // steep exit

    k1.time = 1.0; k1.value = 100.0f;
    k1.out_mode           = InterpolationMode::Bezier;
    k1.in_tangent_time    = 0.333;
    k1.in_tangent_value   = 0.0f;   // flat entry

    curve.keyframes = {k0, k1};

    // Boundary values must be exact
    check_true(nearly_eq(curve.evaluate(0.0), 0.0f, 1e-4f),   "S5: Bezier at t=0 → 0");
    check_true(nearly_eq(curve.evaluate(1.0), 100.0f, 1e-4f), "S5: Bezier at t=1 → 100");

    // Mid value should be NOT equal to 50 (non-uniform due to tangents)
    const float vmid = curve.evaluate(0.5);
    check_true(vmid != 50.0f, "S5: Bezier Hermite mid ≠ linear mid (tangents affect shape)");
}

// ---------------------------------------------------------------------------
// Section 6: AnimatableProperty — static mode + version invalidation
// ---------------------------------------------------------------------------
void test_animatable_property_static() {
    using namespace tachyon::properties;

    AnimatableProperty<float> opacity("Opacity", 0.5f);
    PropertyEvaluationContext ctx{0.0, 0};

    check_true(!opacity.is_animated(), "S6: static property is not animated");
    check_true(nearly_eq(opacity.sample(ctx), 0.5f), "S6: static sample");

    const uint64_t hash_before = opacity.hash_identity(ctx);

    // Mutate: version should change → hash must differ
    opacity.set_static(0.9f);
    const uint64_t hash_after = opacity.hash_identity(ctx);
    check_true(hash_before != hash_after, "S6: hash changes after set_static");
    check_true(nearly_eq(opacity.sample(ctx), 0.9f), "S6: sample after set_static");

    // Static property hash is time-invariant
    PropertyEvaluationContext ctx2{99.0, 0};
    check_true(opacity.hash_identity(ctx) == opacity.hash_identity(ctx2),
               "S6: static property hash is time-invariant");
}

// ---------------------------------------------------------------------------
// Section 7: AnimatableProperty — animated mode + sampling + hash identity
// ---------------------------------------------------------------------------
void test_animatable_property_animated() {
    using namespace tachyon::properties;
    using namespace tachyon::animation;

    AnimatableProperty<float> ramp("Ramp", 0.0f);
    ramp.add_keyframe(0.0, 0.0f);
    ramp.add_keyframe(2.0, 200.0f);

    check_true(ramp.is_animated(), "S7: property becomes animated after add_keyframe");

    PropertyEvaluationContext ctx0{0.0, 0};
    PropertyEvaluationContext ctx1{1.0, 0};
    PropertyEvaluationContext ctx2{2.0, 0};

    check_true(nearly_eq(ramp.sample(ctx0), 0.0f),   "S7: sample at t=0");
    check_true(nearly_eq(ramp.sample(ctx1), 100.0f), "S7: sample at t=1 (midpoint)");
    check_true(nearly_eq(ramp.sample(ctx2), 200.0f), "S7: sample at t=2");

    // Hash at t=0.0 and t=1.999 (same segment) = same
    const uint64_t h0 = ramp.hash_identity(ctx0);
    const uint64_t h1 = ramp.hash_identity(ctx1);
    // Both are in the [0..2] segment — may or may not be equal depending on impl.
    // What we guarantee: hash is deterministic (same call → same result).
    check_true(ramp.hash_identity(ctx0) == h0, "S7: hash is deterministic");
    check_true(ramp.hash_identity(ctx1) == h1, "S7: hash is deterministic (t=1)");

    // Add a new keyframe → version bumps → old hashes are no longer valid
    ramp.add_keyframe(3.0, 300.0f);
    const uint64_t h0_new = ramp.hash_identity(ctx0);
    check_true(h0_new != h0, "S7: adding keyframe invalidates existing hash");
}

// ---------------------------------------------------------------------------
// Section 8: AnimatableProperty — Vector3 typed curve
// ---------------------------------------------------------------------------
void test_animatable_property_vector3() {
    using namespace tachyon::properties;
    using namespace tachyon::math;
    using namespace tachyon::animation;

    AnimatableProperty<Vector3> pos("Position", Vector3::zero());
    pos.add_keyframe(0.0, Vector3(0, 0, 0));
    pos.add_keyframe(1.0, Vector3(10, 20, 30));

    PropertyEvaluationContext ctx_mid{0.5, 0};
    const Vector3 mid = pos.sample(ctx_mid);

    check_true(nearly_eq_v3(mid, Vector3(5, 10, 15)),
               "S8: Vector3 linear interp at t=0.5 → (5, 10, 15)");

    // Static reset
    pos.set_static(Vector3(99, 0, 0));
    check_true(!pos.is_animated(), "S8: set_static makes property non-animated");
    check_true(nearly_eq_v3(pos.sample(ctx_mid), Vector3(99, 0, 0)),
               "S8: Vector3 static value after reset");
}

// ---------------------------------------------------------------------------
// Section 9: PropertyGroup — declare, sample, factory helpers
// ---------------------------------------------------------------------------
void test_property_group() {
    using namespace tachyon::properties;
    using namespace tachyon::math;

    // --- Manual group ---
    PropertyGroup g("TestGroup");
    g.declare_static<float>("opacity", 0.8f);
    g.declare_static<Vector3>("position", {1, 2, 3});

    PropertyEvaluationContext ctx{0.0, 0};
    check_true(g.has("opacity"),   "S9: group has 'opacity'");
    check_true(g.has("position"),  "S9: group has 'position'");
    check_true(!g.has("foobar"),   "S9: group does not have 'foobar'");

    check_true(nearly_eq(g.sample<float>("opacity", ctx), 0.8f),
               "S9: group sample<float> opacity");
    check_true(nearly_eq_v3(g.sample<Vector3>("position", ctx), {1, 2, 3}),
               "S9: group sample<Vector3> position");

    // Mutate via get<>
    g.get<float>("opacity").set_static(0.5f);
    check_true(nearly_eq(g.sample<float>("opacity", ctx), 0.5f),
               "S9: group sample after mutation");

    // --- Transform3 factory ---
    auto xform = make_transform3_group("xform");
    check_true(xform.has("position"),       "S9: transform3 group has position");
    check_true(xform.has("rotation_euler"), "S9: transform3 group has rotation_euler");
    check_true(xform.has("scale"),          "S9: transform3 group has scale");
    check_true(nearly_eq_v3(xform.sample<Vector3>("scale", ctx), Vector3::one()),
               "S9: transform3 default scale is (1,1,1)");

    // --- Camera factory ---
    auto cam = make_camera_group("cam");
    check_true(cam.has("fov_y_deg"),  "S9: camera group has fov_y_deg");
    check_true(cam.has("aperture"),   "S9: camera group has aperture");
    check_true(nearly_eq(cam.sample<float>("fov_y_deg", ctx), 60.0f),
               "S9: camera default fov_y_deg is 60");
}

// ---------------------------------------------------------------------------
// Section 10: Boundary conditions
// ---------------------------------------------------------------------------
void test_boundary_conditions() {
    using namespace tachyon::animation;

    // Empty curve → default-constructed value
    {
        AnimationCurve<float> empty;
        check_true(nearly_eq(empty.evaluate(0.5), 0.0f),
                   "S10: empty curve evaluate returns T{}");
    }

    // Single keyframe → always returns that value
    {
        AnimationCurve<float> single;
        single.keyframes = {{ 0.5, 42.0f }};
        check_true(nearly_eq(single.evaluate(0.0),  42.0f), "S10: single kf clamps low");
        check_true(nearly_eq(single.evaluate(0.5),  42.0f), "S10: single kf at exact time");
        check_true(nearly_eq(single.evaluate(99.0), 42.0f), "S10: single kf clamps high");
    }

    // Out-of-range evaluation clamps
    {
        AnimationCurve<float> c;
        c.keyframes = {
            { 1.0, 10.0f },
            { 2.0, 20.0f }
        };
        check_true(nearly_eq(c.evaluate(0.0), 10.0f), "S10: below-range → first kf");
        check_true(nearly_eq(c.evaluate(3.0), 20.0f), "S10: above-range → last kf");
    }

    // Multi-segment: correct segment selection
    {
        AnimationCurve<float> c;
        c.keyframes = {
            { 0.0, 0.0f,  InterpolationMode::Linear, InterpolationMode::Linear, EasingPreset::None },
            { 1.0, 10.0f, InterpolationMode::Linear, InterpolationMode::Linear, EasingPreset::None },
            { 2.0, 30.0f, InterpolationMode::Linear, InterpolationMode::Linear, EasingPreset::None },
        };
        // Segment [1, 2]: range is 10→30, so at t=1.5 → 20
        check_true(nearly_eq(c.evaluate(1.5), 20.0f), "S10: multi-segment midpoint");
        // Segment [0, 1]: at t=0.5 → 5
        check_true(nearly_eq(c.evaluate(0.5), 5.0f),  "S10: multi-segment first segment");
    }
}

} // anonymous namespace

// ---------------------------------------------------------------------------
// Entry point (linked from test_main.cpp)
// ---------------------------------------------------------------------------
bool run_property_tests() {
    g_failures = 0;

    std::cout << "--- Property System Tests ---\n";

    test_constant_property();
    test_curve_linear_golden();
    test_curve_hold();
    test_curve_easing();
    test_curve_bezier_hermite();
    test_animatable_property_static();
    test_animatable_property_animated();
    test_animatable_property_vector3();
    test_property_group();
    test_boundary_conditions();

    if (g_failures == 0) {
        std::cout << "  all property tests passed\n";
    }

    return g_failures == 0;
}
