#include "src/runtime/core/serialization/tbf_migration.h"

namespace tachyon::runtime {

CompiledScene TBFMigration::migrate(const CompiledScene& scene, std::uint16_t from_version) {
    CompiledScene migrated = scene;
    std::uint16_t ver = from_version;
    
    if (ver == 1) {
        // Migrate layers from v1 to v2
        for (auto& comp : migrated.compositions) {
            for (auto& layer : comp.layers) {
                // Re-create with default mask_feather
                layer.mask_feather = 0.0f;
            }
        }
        ver = 2;
    }
    
    if (ver == 2) {
        // Migrate compositions from v2 to v3
        for (auto& comp : migrated.compositions) {
            comp.audio_tracks = {}; // Explicitly initialize
        }
        ver = 3;
    }

    if (ver == 3) {
        // Migrate from v3 to v4: AudioTrackSpec gained playback_speed, pitch_shift, pitch_correct.
        ver = 4;
    }

    if (ver == 4) {
        // Migrate from v4 to v5: Added text extrusion controls to CompiledLayer.
        ver = 5;
    }
    
    migrated.header.version = TBFCodec::current_version();
    return migrated;
}

} // namespace tachyon::runtime
