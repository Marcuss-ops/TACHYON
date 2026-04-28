#include <gtest/gtest.h>

#include "tachyon/core/math/algebra/vector2.h"
#include "tachyon/core/math/algebra/vector3.h"
#include "tachyon/core/math/algebra/matrix4x4.h"
#include "tachyon/core/math/algebra/quaternion.h"
#include "tachyon/core/math/geometry/transform2.h"
#include "tachyon/core/math/geometry/transform3.h"
#include "tachyon/core/math/geometry/projection.h"
#include "tachyon/core/camera/camera_state.h"

#include <cmath>

namespace {

constexpr float kEps = 1e-4f;

bool nearly_equal(float a, float b, float eps = kEps) {
    return std::abs(a - b) <= eps;
}

bool nearly_equal_vec3(const tachyon::math::Vector3& a, const tachyon::math::Vector3& b, float eps = kEps) {
    return nearly_equal(a.x, b.x, eps) && nearly_equal(a.y, b.y, eps) && nearly_equal(a.z, b.z, eps);
}

} // namespace

TEST(Math, Vector3CrossAndForward) {
    using namespace tachyon::math;
    
    const Vector3 cross = Vector3::cross(Vector3(1, 0, 0), Vector3(0, 1, 0));
    EXPECT_TRUE(nearly_equal_vec3(cross, Vector3(0, 0, 1))) << "Vector3 cross product X x Y = +Z (right-handed)";
    EXPECT_TRUE(nearly_equal_vec3(Vector3::forward(), Vector3(0, 0, -1))) << "Vector3 forward is -Z";
}

TEST(Math, QuaternionNormalizationAndRotation) {
    using namespace tachyon::math;
    
    const Quaternion q = Quaternion::from_axis_angle(Vector3::up(), 3.14159265f * 0.5f).normalized();
    EXPECT_NEAR(q.length(), 1.0f, kEps) << "Quaternion normalization";
    
    const Vector3 rotated = q.rotate_vector(Vector3::forward());
    EXPECT_TRUE(nearly_equal_vec3(rotated, Vector3(-1.0f, 0.0f, 0.0f))) << "Quaternion rotates forward toward -X around +Y";
}

TEST(Math, LookAt) {
    using namespace tachyon::math;
    
    const Matrix4x4 view = look_at(Vector3(0, 0, 10), Vector3(0, 0, 9), Vector3::up());
    const Vector3 camera_space_origin = transform_point(view, Vector3(0, 0, 9));
    EXPECT_TRUE(nearly_equal_vec3(camera_space_origin, Vector3(0, 0, -1))) << "look_at keeps camera forward on -Z";
}

TEST(Math, ComposeTrsAndInverse) {
    using namespace tachyon::math;
    
    const Matrix4x4 trs = compose_trs(Vector3(10, 20, 30), Quaternion::from_axis_angle(Vector3::up(), 3.14159265f * 0.5f), Vector3(2, 3, 4));
    const Matrix4x4 inv = inverse_affine(trs);
    const Vector3 p(1, 2, 3);
    const Vector3 world = transform_point(trs, p);
    const Vector3 restored = transform_point(inv, world);
    EXPECT_TRUE(nearly_equal_vec3(restored, p, 2e-3f)) << "compose_trs and inverse_affine round-trip point";
}

TEST(Math, Transform2ToMatrix) {
    using namespace tachyon::math;
    
    Transform2 t2;
    t2.position = {5.0f, 7.0f};
    t2.rotation_rad = 0.0f;
    t2.scale = {2.0f, 3.0f};
    const Vector3 out = transform_point(t2.to_matrix(), Vector3(1.0f, 1.0f, 0.0f));
    EXPECT_TRUE(nearly_equal_vec3(out, Vector3(7.0f, 10.0f, 0.0f))) << "Transform2 compose to matrix";
}

TEST(Math, Transform3ToMatrix) {
    using namespace tachyon::math;
    
    Transform3 t3;
    t3.position = {3.0f, 4.0f, 5.0f};
    t3.rotation = Quaternion::identity();
    t3.scale = {2.0f, 2.0f, 2.0f};
    const Vector3 out = transform_point(t3.to_matrix(), Vector3(1.0f, 1.0f, 1.0f));
    EXPECT_TRUE(nearly_equal_vec3(out, Vector3(5.0f, 6.0f, 7.0f))) << "Transform3 compose to matrix";
}

TEST(Math, PerspectiveProjection) {
    using namespace tachyon::math;
    
    const Projection proj{Projection::Type::Perspective, 1.570796f, 1.0f, 0.1f, 100.0f, -1.0f, 1.0f, -1.0f, 1.0f};
    const Matrix4x4 p = proj.to_matrix();
    EXPECT_NEAR(p[0], 1.0f, kEps) << "Perspective projection x scale";
    EXPECT_NEAR(p[5], 1.0f, kEps) << "Perspective projection y scale";
    EXPECT_NEAR(p[11], -1.0f, kEps) << "Perspective projection keeps clip-space -Z convention";
}

TEST(Math, CameraProjection) {
    using namespace tachyon;
    using namespace tachyon::math;
    
    camera::CameraState cam;
    cam.transform.position = {0, 0, 10};
    cam.aspect = 1.0f;
    cam.fov_y_rad = 1.570796f;
    
    const Vector2 screen_p = cam.project_point(Vector3(0, 0, 0), 1000, 1000);
    EXPECT_NEAR(screen_p.x, 500.0f, 0.25f) << "Projection center point";
    EXPECT_NEAR(screen_p.y, 500.0f, 0.25f) << "Projection center point";
    
    const Vector2 screen_p2 = cam.project_point(Vector3(1, 1, 9), 1000, 1000);
    EXPECT_NEAR(screen_p2.x, 1000.0f, 0.5f) << "Projection frustum edge";
    EXPECT_NEAR(screen_p2.y, 0.0f, 0.5f) << "Projection frustum edge";
    
    const Vector2 screen_p3 = cam.project_point(Vector3(0, 0, 11), 1000, 1000);
    EXPECT_EQ(screen_p3.x, -1.0f) << "Projection rejects point behind camera";
    EXPECT_EQ(screen_p3.y, -1.0f) << "Projection rejects point behind camera";
}

