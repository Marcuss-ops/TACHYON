#include "test_utils.h"
#include <vector>

bool run_catalog_tests();
bool run_glyph_cache_tests();
bool run_text_tests();
bool run_audio_trim_tests();
bool run_audio_pitch_correct_tests();
bool run_transition_builder_tests();
bool run_text_preset_tests();
bool run_text_animator_preset_registry_tests();
bool run_sfx_contract_tests();
bool run_transition_preset_registry_tests();
bool run_background_kind_registry_tests();
bool run_background_preset_registry_tests();
namespace tachyon { bool run_preset_audit_tests(); }

int main(int argc, char** argv) {
    using namespace tachyon::test;
    std::vector<TestCase> tests = {
        {"catalog", run_catalog_tests},
        {"glyph_cache", run_glyph_cache_tests},
        {"text", run_text_tests},
        {"audio_trim", run_audio_trim_tests},
        {"audio_pitch_correct", run_audio_pitch_correct_tests},
        {"transition_builder", run_transition_builder_tests},
        {"text_preset", run_text_preset_tests},
        {"text_animator_preset_registry", run_text_animator_preset_registry_tests},
        {"sfx", run_sfx_contract_tests},
        {"transition_preset_registry", run_transition_preset_registry_tests},
        {"background_kind_registry", run_background_kind_registry_tests},
        {"background_preset_registry", run_background_preset_registry_tests},
        {"audit", tachyon::run_preset_audit_tests},
    };

    return run_test_suite(argc, argv, tests);
}
