#include "tachyon/camera_cut_contract.h"
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

} // namespace

bool run_camera_cuts_tests() {
    g_failures = 0;

    using namespace tachyon;

    // Empty resolver returns nullopt
    {
        CameraCutTimeline resolver;
        auto cam = resolver.active_camera_at(0.0f);
        check_true(!cam.has_value(), "empty resolver returns nullopt");
    }

    // Single cut: active inside, before, after
    {
        CameraCutTimeline resolver;
        resolver.add_cut({"cam1", 0.0, 5.0});

        auto before = resolver.active_camera_at(-1.0f);
        check_true(!before.has_value(), "before first cut nullopt");

        auto inside = resolver.active_camera_at(2.0f);
        check_true(inside.has_value() && *inside == "cam1", "inside single cut");

        auto after = resolver.active_camera_at(5.001f);
        check_true(!after.has_value(), "after single cut nullopt");

        auto end = resolver.active_camera_at(5.0f);
        check_true(!end.has_value(), "at end boundary exclusive");
    }

    // Multiple cuts: binary search finds correct cut
    {
        CameraCutTimeline resolver;
        resolver.add_cut({"camA", 0.0, 2.0});
        resolver.add_cut({"camB", 2.0, 4.0});
        resolver.add_cut({"camC", 4.0, 6.0});

        check_true(*resolver.active_camera_at(0.5f) == "camA", "finds camA");
        check_true(*resolver.active_camera_at(2.5f) == "camB", "finds camB");
        check_true(*resolver.active_camera_at(4.5f) == "camC", "finds camC");

        check_true(*resolver.active_camera_at(0.0f) == "camA", "boundary start of camA");
        check_true(*resolver.active_camera_at(1.999f) == "camA", "boundary near end of camA");
    }

    // Validation: empty is valid
    {
        CameraCutTimeline resolver;
        std::string error;
        check_true(resolver.validate(error), "empty resolver is valid");
    }

    // Validation: non-empty camera IDs
    {
        CameraCutTimeline resolver;
        resolver.add_cut({"", 0.0, 1.0});
        std::string error;
        check_true(!resolver.validate(error), "empty camera id fails validation");
    }

    // Validation: overlapping cuts fail
    {
        CameraCutTimeline resolver;
        resolver.add_cut({"cam1", 0.0, 3.0});
        resolver.add_cut({"cam2", 2.0, 5.0});
        std::string error;
        check_true(!resolver.validate(error), "overlapping cuts fail validation");
    }

    // Validation: out-of-order start times fail
    {
        CameraCutTimeline resolver;
        resolver.add_cut({"cam1", 2.0, 3.0});
        resolver.add_cut({"cam2", 0.0, 1.0});
        std::string error;
        check_true(!resolver.validate(error), "out-of-order cuts fail validation");
    }

    // Validation: valid ordered non-overlapping passes
    {
        CameraCutTimeline resolver;
        resolver.add_cut({"cam1", 0.0, 2.0});
        resolver.add_cut({"cam2", 2.0, 4.0});
        resolver.add_cut({"cam3", 4.0, 6.0});
        std::string error;
        check_true(resolver.validate(error), "valid ordered cuts pass");
    }

    // CameraCut::contains
    {
        CameraCut cut{"cam1", 1.0, 3.0};
        check_true(cut.contains(1.0), "contains inclusive start");
        check_true(cut.contains(2.0), "contains middle");
        check_true(!cut.contains(3.0), "excludes end boundary");
        check_true(!cut.contains(0.5), "excludes before");
        check_true(!cut.contains(3.5), "excludes after");
    }

    return g_failures == 0;
}
