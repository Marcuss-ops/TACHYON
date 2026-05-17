#include "test_utils.h"
#include "image_comparison.h"
#include "tachyon/scene/builder.h"

namespace tachyon::test {

bool run_golden_smoke_test() {
    using namespace tachyon::scene;
    
    // Create a very simple deterministic scene
    auto scene = Composition("smoke_test")
        .size(256, 256)
        .duration(1.0)
        .fps(30)
        .clear(ColorSpec{255, 0, 0, 255}) // Solid red
        .build_scene();
        
    // First run with TACHYON_UPDATE_GOLDEN=1 will create the baseline.
    // Subsequent runs will compare against it.
    return verify_golden_frame("smoke_red_square", scene, 0);
}

} // namespace tachyon::test
