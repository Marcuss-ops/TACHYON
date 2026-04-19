#include "tachyon/core/math/vector2.h"
#include "tachyon/core/math/vector3.h"
#include "tachyon/core/math/matrix4x4.h"
#include "tachyon/core/math/quaternion.h"
#include "tachyon/core/math/transform3.h"
#include "tachyon/core/camera/camera_state.h"

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

} // namespace

bool run_math_tests() {
    using namespace tachyon::math;

    // Vector3 tests
    {
        Vector3 a(1, 2, 3);
        Vector3 b(4, 5, 6);
        Vector3 c = a + b;
        check_true(c.x == 5 && c.y == 7 && c.z == 9, "Vector3 addition");

        float d = Vector3::dot(Vector3(1, 0, 0), Vector3(0, 1, 0));
        check_true(d == 0.0f, "Vector3 dot product orthogonal");

        Vector3 cross = Vector3::cross(Vector3(1, 0, 0), Vector3(0, 1, 0));
        check_true(cross.z == 1.0f, "Vector3 cross product X x Y = Z (right-handed)");
    }

    // Matrix tests
    {
        Matrix4x4 identity = Matrix4x4::identity();
        check_true(identity[0] == 1.0f && identity[5] == 1.0f && identity[10] == 1.0f && identity[15] == 1.0f, "Matrix identity");

        Matrix4x4 translation = Matrix4x4::translation({10, 20, 30});
        check_true(translation[12] == 10.0f && translation[13] == 20.0f && translation[14] == 30.0f, "Matrix translation storage (col-major)");

        Matrix4x4 scale = Matrix4x4::scaling({2, 3, 4});
        check_true(scale[0] == 2.0f && scale[5] == 3.0f && scale[10] == 4.0f, "Matrix scaling storage");
    }

    // Camera and MVP Pipeline tests
    {
        using namespace tachyon::camera;
        CameraState cam;
        // Move camera 10 units back on Z
        cam.transform.position = {0, 0, 10};
        cam.aspect = 1.0f; // Square viewport for easy math
        cam.fov_y_rad = 1.570796f; // 90 degrees fov_y (tan(45) = 1)
        
        // A point at (0, 0, 0) should project to the center of any viewport
        Vector3 world_p(0, 0, 0);
        Vector2 screen_p = cam.project_point(world_p, 1000, 1000);
        check_true(std::abs(screen_p.x - 500.0f) < 0.1f && std::abs(screen_p.y - 500.0f) < 0.1f, "Projection: center point");
        
        // A point at (1, 1, 9) is 1 unit forward and 1 unit top-right of center line
        // At Z=-1 in camera space, tan(45)=1 means edge of frustum is at X=1, Y=1
        // So (1, 1, 9) world -> (1, 1, -1) camera -> (1, 1) NDC -> (1000, 0) screen (Raster Y is down)
        Vector3 world_p2(1, 1, 9);
        Vector2 screen_p2 = cam.project_point(world_p2, 1000, 1000);
        check_true(std::abs(screen_p2.x - 1000.0f) < 0.1f && std::abs(screen_p2.y - 0.0f) < 0.1f, "Projection: frustum edge");
        
        // A point behind the camera should be rejected
        Vector3 world_p3(0, 0, 11);
        Vector2 screen_p3 = cam.project_point(world_p3, 1000, 1000);
        check_true(screen_p3.x == -1.0f && screen_p3.y == -1.0f, "Projection: culled behind far plane");
    }

    return g_failures == 0;
}
