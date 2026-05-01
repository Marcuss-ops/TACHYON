#include "tachyon/presets/sfx/sfx_builders.h"
#include "tachyon/presets/sfx/sfx_params.h"
#include <cassert>

bool run_sfx_contract_tests() {
    tachyon::presets::SfxParams p;
    p.category = tachyon::presets::SfxCategory::TypeWriting;
    p.variant = 2;
    p.in_point = 1.0;
    p.out_point = 4.0;
    p.volume = 0.8f;
    auto spec = tachyon::presets::build_sfx(p);
    assert(!spec.source_path.empty());
    assert(spec.in_point == 1.0);
    assert(spec.out_point == 4.0);
    assert(spec.volume == 0.8f);

    auto shortcut_spec = tachyon::presets::build_sfx(tachyon::presets::SfxCategory::MoneySound, 2.0, 0.5f);
    assert(!shortcut_spec.source_path.empty());
    assert(shortcut_spec.in_point == 2.0);
    assert(shortcut_spec.volume == 0.5f);

    return true;
}
