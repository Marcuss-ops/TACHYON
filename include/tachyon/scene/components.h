#pragma once

#include "tachyon/scene/builder.h"

namespace tachyon::scene {

/**
 * @brief Standard title card component.
 * Demonstrates the convention of factory functions returning LayerSpec.
 */
inline LayerSpec title_card(std::string text, double in, double out) {
    return LayerBuilder("title")
        .type("text")
        .text(std::move(text))
        .in(in)
        .out(out)
        .opacity(anim::lerp(0, 1, 0.4))
        .build();
}

/**
 * @brief Caption track component stub.
 */
inline LayerSpec caption_track(std::string srt_path) {
    return LayerBuilder("captions")
        .type("text")
        .subtitle_path(std::move(srt_path))
        .build();
}

/**
 * @brief Generic composition factory.
 */
inline CompositionSpec make_short(std::string voice, std::string srt,
                                  std::function<void(CompositionBuilder&)> fn) {
    CompositionBuilder cb("short");
    cb.duration(60.0);
    cb.audio(std::move(voice));
    cb.layer("captions", [srt = std::move(srt)](LayerBuilder& l) {
        l.type("text")
         .subtitle_path(std::move(srt));
    });
    fn(cb);
    return std::move(cb).build();
}

} // namespace tachyon::scene
