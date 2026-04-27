#include "tachyon/renderer2d/effects/particle_effects.h"
#include <cmath>

bool run_particle_system_tests() {
    using namespace tachyon::renderer2d;
    
    ParticleSystem ps;
    ps.spawn_rate = 10;
    ps.lifetime = 2.0f;
    ps.initial_velocity = {0.0f, 10.0f};
    
    // Test spawn at t=0
    ps.update(0.0f);
    if (ps.particles.size() != 0) return false; // No time elapsed yet
    
    // Test spawn after 0.1s
    ps.update(0.1f);
    size_t count_t0 = ps.particles.size();
    if (count_t0 == 0) return false; // Should have spawned particles
    
    // Test lifetime: particles should expire after lifetime seconds
    ps.update(ps.lifetime + 0.1f);
    for (const auto& p : ps.particles) {
        if (p.age <= ps.lifetime) return false;
    }
    
    // Test position at t=1s (simplified: velocity * time)
    ps.reset();
    ps.update(1.0f);
    for (const auto& p : ps.particles) {
        float expected_y = p.initial_pos.y + p.initial_velocity.y * 1.0f;
        if (std::abs(p.pos.y - expected_y) > 0.1f) return false;
    }
    
    return true;
}
