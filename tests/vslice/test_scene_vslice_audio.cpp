#include "tachyon/scene/builder.h"
#include "tachyon/presets/text/fluent.h"

extern "C" void build_scene(tachyon::SceneSpec& out) {
    using namespace tachyon;
    out = scene::SceneBuilder()
        .project("vslice_audio", "Audio Muxing Test")
        .composition("main", [](scene::CompositionBuilder& c) {
            c.size(1280, 720).fps(30).duration(5.0);
            c.audio("assets/intro_music.mp3");
            c.layer(presets::text::headline("AUDIO TEST").center().build());
        }).build();
}
