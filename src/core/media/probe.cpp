#include "tachyon/core/media/probe.h"
#include <filesystem>
#include <iostream>

#if __has_include(<libavcodec/avcodec.h>) && __has_include(<libavformat/avformat.h>) && __has_include(<libavutil/channel_layout.h>)
extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/channel_layout.h>
}
#define TACHYON_HAS_FFMPEG 1
#endif

namespace tachyon::media {

core::MediaResult<FullMetadata> MediaProbe::probe_file(const std::filesystem::path& path) {
    if (!std::filesystem::exists(path)) {
        return core::MediaResult<FullMetadata>::failure(
            core::MediaError(core::MediaErrorCode::IO, "File not found: " + path.string())
                .with_path(path.string())
        );
    }
    return probe_full(path);
}

core::MediaResult<FullMetadata> MediaProbe::probe_full(const std::filesystem::path& path) {
#if !defined(TACHYON_HAS_FFMPEG)
    return core::MediaResult<FullMetadata>::failure(
        core::MediaError(core::MediaErrorCode::Init, "FFmpeg support is not compiled into this build")
    );
#else
    AVFormatContext* fmt_ctx = nullptr;
    if (avformat_open_input(&fmt_ctx, path.string().c_str(), nullptr, nullptr) < 0) {
        return core::MediaResult<FullMetadata>::failure(
            core::MediaError(core::MediaErrorCode::Decode, "FFmpeg failed to open input")
                .with_path(path.string())
                .with_stage("probe_open")
        );
    }

    if (avformat_find_stream_info(fmt_ctx, nullptr) < 0) {
        avformat_close_input(&fmt_ctx);
        return core::MediaResult<FullMetadata>::failure(
            core::MediaError(core::MediaErrorCode::Decode, "FFmpeg failed to find stream info")
                .with_path(path.string())
                .with_stage("probe_stream_info")
        );
    }

    FullMetadata meta;
    meta.path = path.string();
    meta.format = (fmt_ctx->iformat && fmt_ctx->iformat->name) ? fmt_ctx->iformat->name : "unknown";
    meta.duration_seconds = (fmt_ctx->duration != AV_NOPTS_VALUE) 
        ? static_cast<double>(fmt_ctx->duration) / AV_TIME_BASE 
        : 0.0;
    meta.bit_rate = fmt_ctx->bit_rate;
    
    // Get file size
    try {
        meta.size_bytes = static_cast<std::int64_t>(std::filesystem::file_size(path));
    } catch (...) {
        meta.size_bytes = 0;
    }

    for (unsigned int i = 0; i < fmt_ctx->nb_streams; i++) {
        AVStream* stream = fmt_ctx->streams[i];
        AVCodecParameters* codec_par = stream->codecpar;

        if (codec_par->codec_type == AVMEDIA_TYPE_VIDEO && !meta.video) {
            VideoMetadata v;
            const AVCodec* codec = avcodec_find_decoder(codec_par->codec_id);
            v.codec = codec ? codec->name : "unknown";
            v.width = codec_par->width;
            v.height = codec_par->height;
            if (stream->avg_frame_rate.den > 0) {
                v.fps = av_q2d(stream->avg_frame_rate);
            }
            v.duration_seconds = (stream->duration != AV_NOPTS_VALUE)
                ? static_cast<double>(stream->duration) * av_q2d(stream->time_base)
                : meta.duration_seconds;
            v.bit_rate = codec_par->bit_rate;
            meta.video = v;
        } else if (codec_par->codec_type == AVMEDIA_TYPE_AUDIO && !meta.audio) {
            AudioMetadata a;
            const AVCodec* codec = avcodec_find_decoder(codec_par->codec_id);
            a.codec = codec ? codec->name : "unknown";
            a.sample_rate = codec_par->sample_rate;
            
            // Channel layout extraction (FFmpeg 5.1+)
            a.channels = codec_par->ch_layout.nb_channels;
            
            a.duration_seconds = (stream->duration != AV_NOPTS_VALUE)
                ? static_cast<double>(stream->duration) * av_q2d(stream->time_base)
                : meta.duration_seconds;
            a.bit_rate = codec_par->bit_rate;
            
            // Bit depth estimation
            switch (codec_par->format) {
                case AV_SAMPLE_FMT_U8:
                case AV_SAMPLE_FMT_U8P: a.bit_depth = 8; break;
                case AV_SAMPLE_FMT_S16:
                case AV_SAMPLE_FMT_S16P: a.bit_depth = 16; break;
                case AV_SAMPLE_FMT_S32:
                case AV_SAMPLE_FMT_S32P: a.bit_depth = 32; break;
                case AV_SAMPLE_FMT_FLT:
                case AV_SAMPLE_FMT_FLTP: a.bit_depth = 32; break;
                case AV_SAMPLE_FMT_DBL:
                case AV_SAMPLE_FMT_DBLP: a.bit_depth = 64; break;
                default: a.bit_depth = 0; break;
            }
            
            meta.audio = a;
        }
    }

    avformat_close_input(&fmt_ctx);
    return core::MediaResult<FullMetadata>::success(std::move(meta));
#endif
}

} // namespace tachyon::media
