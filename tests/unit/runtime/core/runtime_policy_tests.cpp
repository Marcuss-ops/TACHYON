#include "test_utils.h"
#include "tachyon/runtime/policy/worker_policy.h"
#include "tachyon/runtime/policy/surface_pool_policy.h"
#include "tachyon/runtime/policy/telemetry_policy.h"
#include <cassert>
#include <iostream>

namespace tachyon::runtime {

bool test_worker_policy() {
    WorkerPolicy policy;
    
    // Default resolution
    assert(policy.resolve(8) == 8);
    
    // Overrides
    policy.requested_workers = 4;
    assert(policy.resolve(8) == 4);
    
    // Min/Max constraints
    policy.requested_workers = 0;
    policy.min_workers = 2;
    assert(policy.resolve(8) == 2);
    
    policy.max_workers = 6;
    assert(policy.resolve(12) == 6);
    
    return true;
}

bool test_surface_pool_policy() {
    SurfacePoolPolicy policy;
    
    // Default heuristic: 2 * workers
    assert(policy.resolve(1920, 1080, 4) == 10); // 2*4 + extra(2)
    
    // HD threshold (2K)
    assert(policy.resolve(2560, 1440, 4) == 8); // 1.5*4 + extra(2)
    
    // 4K threshold
    assert(policy.resolve(3840, 2160, 4) == 6); // 1.0*4 + extra(2)
    
    // Min/Max constraints
    policy.min_surfaces = 12;
    assert(policy.resolve(3840, 2160, 4) == 12);
    
    policy.max_surfaces = 14;
    assert(policy.resolve(1920, 1080, 8) == 14);
    
    return true;
}

bool test_telemetry_policy() {
    TelemetryPolicy policy;
    
    // Default: Summary (enabled)
    assert(policy.should_sample_process());
    
    // Manual disable
    policy.level = TelemetryLevel::Off;
    assert(!policy.should_sample_process());
    
    return true;
}

} // namespace tachyon::runtime

bool run_runtime_policy_tests() {
    bool ok = true;
    ok &= tachyon::runtime::test_worker_policy();
    ok &= tachyon::runtime::test_surface_pool_policy();
    ok &= tachyon::runtime::test_telemetry_policy();
    return ok;
}
