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
inline LayerSpec caption_track(std::string srt_path, std::string style = "viral") {
    return LayerBuilder("captions")
        .type("text")
        .prop("subtitle_path", srt_path)
        .prop("style", style)
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
         .prop("subtitle_path", srt)
         .prop("style", "viral");
    });
    fn(cb);
    return std::move(cb).build();
}

} // namespace tachyon::scene
