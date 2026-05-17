#include <gtest/gtest.h>
#include "tachyon/core/c_api.h"
#include <cstring>

namespace tachyon::test {

bool run_c_api_smoke_test() {
    // 1. Version test
    const char* version = tachyon_version();
    if (!version || std::strlen(version) == 0) {
        std::cerr << "tachyon_version() returned invalid string\n";
        return false;
    }

    // 2. Init test (idempotent)
    if (tachyon_init() != 0) {
        std::cerr << "tachyon_init() failed (1)\n";
        return false;
    }
    if (tachyon_init() != 0) {
        std::cerr << "tachyon_init() failed (2)\n";
        return false;
    }

    // 3. Run invalid args test
    char err[1024] = {};
    if (tachyon_run(0, nullptr, err, sizeof(err)) == 0) {
        std::cerr << "tachyon_run(0, nullptr) should have failed\n";
        return false;
    }
    if (std::strlen(err) == 0) {
        std::cerr << "tachyon_run failure should have provided an error message\n";
        return false;
    }

    // 4. Run version command test
    const char* args[] = {"tachyon", "version"};
    if (tachyon_run(2, args, err, sizeof(err)) != 0) {
        std::cerr << "tachyon_run(version) failed: " << err << "\n";
        return false;
    }

    // 5. Render test (smoke)
    TachyonRenderOptions render_opts;
    tachyon_init_render_options(&render_opts);
    render_opts.preset_id = "tachyon.scene.modern_grid";
    render_opts.start_frame = 0;
    render_opts.end_frame = 1; // 2 frames (0 and 1)
    render_opts.output_path = "c_api_test.mp4";
    
    // Clear error buffer before testing
    std::memset(err, 0, sizeof(err));

    int rc = tachyon_render(&render_opts, err, sizeof(err));
    if (rc != 0) {
        std::cerr << "tachyon_render(tachyon.scene.modern_grid) failed: " << err << "\n";
        return false;
    }

    return true;
}

} // namespace tachyon::test
