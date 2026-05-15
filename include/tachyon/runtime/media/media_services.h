#pragma once

#include "tachyon/core/media/media_interfaces.h"
#include "tachyon/core/media/clip_processor.h"
#include "tachyon/core/media/overlay_merger.h"
#include "tachyon/core/media/audio_extract.h"
#include "tachyon/core/media/video_concat.h"

namespace tachyon::runtime::media {

/**
 * @brief Bundle of media services required by the pipeline.
 * Using references to enforce that services must be provided by the caller.
 */
struct MediaServices {
    core::media::IMediaProbe& probe;
    core::media::IClipProcessor& clip_processor;
    core::media::IOverlayMerger& overlay_merger;
    core::media::IAudioExtractor& audio_extractor;
    core::media::IAudioAnalyzer& audio_analyzer;
    core::media::IVideoConcat& video_concat;
};

} // namespace tachyon::runtime::media
