// Example: TACHYON Remotion-style C++ API
// This file demonstrates the new fluent API for creating scenes

#include "tachyon/scene/builder.h"
#include "tachyon/presets/text/fluent.h"
#include "tachyon/presets/background/fluent.h"
#include "tachyon/presets/audio/fluent.h"

// ---------------------------------------------------------------------------
// Example 1: Simple text scene with fluent API
// ---------------------------------------------------------------------------

tachyon::SceneSpec create_simple_text_scene() {
    using namespace tachyon;
    using namespace tachyon::presets;
    
    return SceneBuilder()
        .project("proj_001", "Simple Text Scene")
        .composition("main", [](CompositionBuilder& c) {
            c.size(1920, 1080)
             .fps(30)
             .duration(8.0)
             .background(BackgroundSpec::from_string("#0d1117"));
             
            // Add background using fluent API
            c.layer(background::aura()
                .width(1920)
                .height(1080)
                .duration(8.0)
                .seed(42)
                .palette(background::palettes::neon_night())
                .grain(0.12)
                .build());
            
            // Add text using fluent API
            c.layer(text::headline("Hello TACHYON")
                .font("Inter")
                .size(96)
                .color({255, 255, 255, 255})
                .center()
                .build());
        })
        .build();
}

// ---------------------------------------------------------------------------
// Example 2: Full Remotion-style scene
// ---------------------------------------------------------------------------

extern "C" void build_scene(tachyon::SceneSpec& out) {
    using namespace tachyon;
    using namespace tachyon::presets;
    
    out = SceneBuilder()
        .project("video_001", "My Video")
        .composition("main", [](CompositionBuilder& c) {
            c.size(1920, 1080)
             .fps(30)
             .duration(8.0)
             .background(BackgroundSpec::from_string("#0d1117"));
             
            // Animated background using procedural preset
            c.layer(background::aura()
                .width(1920)
                .height(1080)
                .duration(8.0)
                .seed(42)
                .palette(background::palettes::neon_night())
                .grain(0.12)
                .build());
             
            // Audio track with fluent API
            c.audio(audio::track("voice")
                .source("voiceover.wav")
                .volume(1.0)
                .fade_in(0.15)
                .normalize_lufs(-14.0)
                .build());
             
            // Headline text
            c.layer(text::headline("TACHYON NATIVE")
                .font_size(96)
                .center()
                .color({238, 242, 248, 255})
                .build());
        })
        .build();
}

// ---------------------------------------------------------------------------
// Example 3: Multiple text animations
// ---------------------------------------------------------------------------

tachyon::SceneSpec create_multi_animation_scene() {
    using namespace tachyon;
    using namespace tachyon::presets;
    
    return SceneBuilder()
        .project("proj_002", "Multi Animation")
        .composition("main", [](CompositionBuilder& c) {
            c.size(1920, 1080).fps(30).duration(10.0);
            
            // Text with multiple animators using the animator builder
            text::TextBuilder builder = text::headline("Animated Text");
            builder.size(88)
                   .center()
                   .animate(text::fade_up().build())
                   .animate(text::blur_to_focus().build());
            
            c.layer(builder.build());
        })
        .build();
}
