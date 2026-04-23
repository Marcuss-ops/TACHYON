#pragma once

#include "tachyon/core/spec/schema/audio/audio_spec.h"
#include "tachyon/audio/audio_encoder.h"

#include <memory>
#include <vector>
#include <string>

namespace tachyon {
namespace audio {

class AudioDecoder;

class AudioExporter {
public:
    AudioExporter();
    ~AudioExporter();

    void add_track(const AudioTrackSpec& track_spec);
    bool export_to(const std::filesystem::path& output_path, const AudioExportConfig& config);

    void clear_tracks();

private:
    struct TrackInstance {
        std::shared_ptr<AudioDecoder> decoder;
        AudioTrackSpec spec;
    };

    std::vector<TrackInstance> m_tracks;

    float evaluate_volume_at_time(const AudioTrackSpec& track, double time) const;
    float evaluate_pan_at_time(const AudioTrackSpec& track, double time) const;
    void apply_pan(float* interleaved_samples, std::size_t sample_count, float pan);

    static constexpr int kTargetSampleRate = 48000;
    static constexpr int kTargetChannels = 2;
};

} // namespace audio
} // namespace tachyon
