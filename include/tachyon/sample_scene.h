#pragma once

#include "tachyon/presets/presets.h"
#include "tachyon/scene/builder.h"
#include <iostream>

/**
 * @file sample_scene.h
 * @brief Example of the new Remotion-like C++ API for Tachyon.
 * 
 * This demonstrates the target API described in the user's request.
 */

namespace tachyon {

/**
 * @brief Build a sample scene using the new fluent API.
 * 
 * This is equivalent to the user's target API:
 * @code
 * extern "C" void build_scene(tachyon::SceneSpec& out) {
 *     out = scene::Scene()
 *         .project("video_001", "My Video")
 *         .composition("main", [](auto& c) {
 *             c.size(1920, 1080)
 *              .fps(30)
 *              .duration(8.0)
 *              .clear({13, 17, 23, 255})
 *              .layer(background::kind_aura()
 *                  .width(1920)
 *                  .height(1080)
 *                  .duration(8.0)
 *                  .seed(42)
 *                  .build())
 *              .audio(audio::track("voice")...)
 *              .layer(text::headline("TACHYON NATIVE")...);
 *         })
 *         .build();
 * }
 * @endcode
 */
inline SceneSpec build_sample_scene() {
    using namespace scene;
    using namespace presets;
    
    return Scene()
        .project("video_001", "My Video")
        .composition("main", [](auto& c) {
            c.size(1920, 1080)
             .fps(30)
             .duration(8.0)
             .clear({13, 17, 23, 255})
             
             .layer(background::kind_aura()
                 .width(1920)
                 .height(1080)
                 .duration(8.0)
                 .seed(42)
                 .build())
             
             .audio(audio::track("voice")
                 .source("voiceover.wav")
                 .volume(1.0)
                 .fade_in(0.15)
                 .normalize_lufs(-14.0))
             
             .layer(text::headline("TACHYON NATIVE")
                 .font("Inter")
                 .size(96)
                 .color({238, 242, 248, 255})
                 .center()
                 .animate(text::make_fade_up_animator("characters_excluding_spaces", 0.6, 0.03)));
        })
        .build();
}

} // namespace tachyon
