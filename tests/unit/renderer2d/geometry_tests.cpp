#include "tachyon/renderer2d/geometry/dirty_region.h"
#include <iostream>

using namespace tachyon::renderer2d;

bool run_geometry_tests() {
    std::cout << "[GeometryTests] Starting...\n";
    bool ok = true;

    // Test: EmptyRects
    {
        IntRect r1{0, 0, 0, 0};
        if (!r1.empty()) { std::cerr << "FAIL: r1 should be empty\n"; ok = false; }

        IntRect r2{10, 10, -5, 100};
        if (!r2.empty()) { std::cerr << "FAIL: r2 should be empty\n"; ok = false; }
    }

    // Test: Intersections
    {
        IntRect r1{0, 0, 100, 100};
        IntRect r2{50, 50, 100, 100};
        IntRect r3{200, 200, 50, 50};

        if (!r1.intersects(r2)) { std::cerr << "FAIL: r1 should intersect r2\n"; ok = false; }
        if (r1.intersects(r3)) { std::cerr << "FAIL: r1 should not intersect r3\n"; ok = false; }

        IntRect intersect = r1.intersection(r2);
        if (!(intersect == IntRect{50, 50, 50, 50})) {
            std::cerr << "FAIL: incorrect intersection\n"; ok = false;
        }
    }

    // Test: Unions
    {
        IntRect r1{0, 0, 50, 50};
        IntRect r2{100, 100, 50, 50};

        IntRect un = r1.united(r2);
        if (!(un == IntRect{0, 0, 150, 150})) {
            std::cerr << "FAIL: incorrect union\n"; ok = false;
        }
    }

    // Test: DirtyRegion AddAndBounds
    {
        DirtyRegion region;
        if (!region.empty()) { std::cerr << "FAIL: region should be empty\n"; ok = false; }

        region.add({0, 0, 10, 10});
        if (region.empty()) { std::cerr << "FAIL: region should not be empty\n"; ok = false; }

        region.add({20, 20, 10, 10});
        IntRect b = region.bounds();
        if (!(b == IntRect{0, 0, 30, 30})) {
            std::cerr << "FAIL: incorrect region bounds\n"; ok = false;
        }
    }

    // Test: DirtyRegion AddUnion
    {
        DirtyRegion region;
        region.add_union({0, 0, 10, 10});
        region.add_union({20, 20, 10, 10});

        if (region.rects().size() != 1) { std::cerr << "FAIL: region should have 1 merged rect\n"; ok = false; }
        if (!(region.bounds() == IntRect{0, 0, 30, 30})) {
            std::cerr << "FAIL: incorrect region bounds after add_union\n"; ok = false;
        }
    }

    if (ok) std::cout << "[GeometryTests] ALL PASSED!\n";
    return ok;
}