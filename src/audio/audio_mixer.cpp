#include "tachyon/audio/audio_mixer.h"
#include "tachyon/audio/dsp_nodes.h"
#include "tachyon/runtime/execution/presentation_clock.h"

namespace tachyon::audio {

AudioMixer::AudioMixer() {
    // Add default limiter to master bus
    m_processor.graph().master().add_node(std::make_shared<LimiterNode>(1.0f));
}

AudioMixer::~AudioMixer() = default;

void AudioMixer::add_track(std::shared_ptr<AudioDecoder> decoder,
                           const AudioTrackMixParams &params,
                           const std::string &bus_id) {
    if (!decoder || !params.enabled) return;

    spec::AudioTrackSpec track_spec;
    track_spec.start_offset_seconds = params.start_offset_seconds;
    track_spec.volume = params.volume;
    track_spec.pan = params.pan;
    track_spec.playback_speed = params.playback_speed;
    track_spec.pitch_shift = params.pitch_shift;
    track_spec.pitch_correct = params.pitch_correct;
    
    m_processor.add_track(std::move(decoder), track_spec, bus_id);
}

void AudioMixer::mix(double startTimeSeconds, double durationSeconds,
                     std::vector<float> &out_stereo_pcm) {
    m_processor.process(startTimeSeconds, durationSeconds, kTargetSampleRate, out_stereo_pcm);
    
    if (m_presentation_clock) {
        // Update the clock with the number of samples we just "played"
        // out_stereo_pcm.size() / 2 is the number of stereo frames
        m_presentation_clock->update_from_audio(out_stereo_pcm.size() / 2);
    }
}

void AudioMixer::clear_tracks() {
    m_processor.clear_tracks();
}

} // namespace tachyon::audio
