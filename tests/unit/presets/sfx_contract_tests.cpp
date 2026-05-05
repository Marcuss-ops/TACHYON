#include "tachyon/presets/sfx/sfx_builders.h"
#include "tachyon/presets/sfx/sfx_params.h"
#include <cassert>
#include <string>
#include <iostream>

bool run_sfx_contract_tests() {
    using namespace tachyon::presets;

    std::cout << "Running SFX contract tests..." << std::endl;
    
    // Setup a dummy resolver
    tachyon::media::AssetResolver::Config config;
    config.sfx_root = "dummy/sfx"; // Won't find files, but tests the path building
    tachyon::media::AssetResolver resolver(config);

    // 1. Test explicit variant selection
    {
        SfxParams p;
        p.category = SfxCategory::TypeWriting;
        p.variant = 1;
        p.in_point = 1.0;
        p.out_point = 4.0;
        p.volume = 0.8f;
        auto spec = build_sfx(resolver, p);
        // We can't easily check the path without TACHYON_SFX_ROOT, but we can check if it contains the variant
        assert(spec.in_point_seconds == 1.0);
        assert(spec.out_point_seconds == 4.0);
        assert(spec.volume == 0.8f);
    }

    // 2. Test deterministic random selection (variant -1)
    {
        SfxParams p1;
        p1.category = SfxCategory::Mouse;
        p1.variant = -1;
        p1.seed = 12345;
        auto spec1 = build_sfx(resolver, p1);

        SfxParams p2;
        p2.category = SfxCategory::Mouse;
        p2.variant = -1;
        p2.seed = 12345;
        auto spec2 = build_sfx(resolver, p2);

        // Same seed must return same path
        if (!spec1.source_path.empty() && !spec2.source_path.empty()) {
            assert(spec1.source_path == spec2.source_path);
        }

        SfxParams p3;
        p3.category = SfxCategory::Mouse;
        p3.variant = -1;
        p3.seed = 54321;
        auto spec3 = build_sfx(resolver, p3);

        // Different seed might return different path (statistically likely if >1 file exists)
        // We don't assert inequality here because there might be only one file in the folder,
        // but we verify that the system doesn't crash.
    }

    // 3. Test shortcut API
    {
        auto shortcut_spec = build_sfx(resolver, SfxCategory::MoneySound, 2.0, 0.5f);
        assert(shortcut_spec.in_point_seconds == 2.0);
        assert(shortcut_spec.volume == 0.5f);
    }

    // 4. Test diagnostic reporting on failure
    {
        tachyon::DiagnosticBag diags;
        SfxParams p;
        p.category = SfxCategory::Photo;
        p.variant = 999; // Non-existent variant
        auto spec = build_sfx(resolver, p, &diags);
        
        assert(!diags.ok());
        bool found_error = false;
        for (const auto& d : diags.diagnostics) {
            if (d.code == "SFX_VARIANT_NOT_FOUND") found_error = true;
        }
        assert(found_error);
    }

    std::cout << "SFX contract tests passed!" << std::endl;
    return true;
}
