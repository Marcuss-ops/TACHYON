#include "tachyon/core/math/vector2.h"
#include "tachyon/core/math/vector3.h"
#include "tachyon/core/math/matrix4x4.h"
#include "tachyon/core/math/quaternion.h"

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

    return g_failures == 0;
}
