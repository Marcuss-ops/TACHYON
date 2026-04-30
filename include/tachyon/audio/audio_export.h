#pragma once

#include "tachyon/core/spec/schema/audio/audio_spec.h"
#include "tachyon/audio/audio_encoder.h"
#include "tachyon/audio/loudness_meter.h"

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
    
    // Get loudness measurement after export
    LoudnessMeasurement get_loudness_measurement() const { return m_loudness_measurement; }

private:
    struct TrackInstance {
        std::shared_ptr<AudioDecoder> decoder;
        AudioTrackSpec spec;
    };

    std::vector<TrackInstance> m_tracks;
    LoudnessMeter m_loudness_meter;
    LoudnessMeasurement m_loudness_measurement;

    float evaluate_volume_at_time(const AudioTrackSpec& track, double time) const;
    float evaluate_pan_at_time(const AudioTrackSpec& track, double time) const;
    float evaluate_fade_at_time(const AudioTrackSpec& track, double time, double chunk_duration) const;
    double get_track_end_time(const AudioTrackSpec& track) const;
    void apply_pan(float* interleaved_samples, std::size_t sample_count, float pan);
    void apply_trim_and_speed(AudioDecoder* decoder, const AudioTrackSpec& spec, 
                             double chunk_start, double chunk_duration, 
                             std::vector<float>& output_buffer);

    static constexpr int kTargetSampleRate = 48000;
    static constexpr int kTargetChannels = 2;
};

} // namespace audio
} // namespace tachyon
