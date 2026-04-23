#include "tachyon/output/av_exporter.h"
#include "tachyon/output/ffmpeg/ffmpeg_utils.h"
#include <fstream>
#include <sstream>
#include <thread>
#include <atomic>
#include <mutex>

namespace tachyon {

class AVExporter::Impl {
public:
    Impl(const AVExportConfig& config) : config_(config), cancelled(false), progress(0.0f) {}
    
    bool initialize(const std::string& output_path) {
        this->output_path = output_path;
        m_last_error.clear();
        
        if (!check_ffmpeg_available()) {
            m_last_error = "FFmpeg not found in PATH";
            return false;
        }
        
        return true;
    }
    
    bool export_video(FrameProvider* video_source) {
        m_last_error.clear();
        if (!video_source) {
            m_last_error = "video source is null";
            return false;
        }
        if (!initialize_video_pipeline()) {
            return false;
        }
        
        const int total_frames = video_source->get_total_frames();
        std::vector<uint8_t> frame_buffer(config_.width * config_.height * 4);
        
        for (int i = 0; i < total_frames && !cancelled; ++i) {
            if (!video_source->get_frame(i, frame_buffer.data(), config_.width * 4)) {
                m_last_error = "Failed to get frame " + std::to_string(i);
                return false;
            }
            
            if (!encode_video_frame(frame_buffer.data())) {
                m_last_error = "Failed to encode frame " + std::to_string(i);
                return false;
            }
            
            progress = static_cast<float>(i) / total_frames;
        }
        
        return finalize_video_pipeline() && !cancelled;
    }
    
    bool export_audio(AudioProvider* audio_source) {
        m_last_error.clear();
        if (!audio_source) {
            m_last_error = "audio source is null";
            return false;
        }
        if (!initialize_audio_pipeline()) {
            return false;
        }
        
        const int total_samples = audio_source->get_total_samples();
        const int buffer_size = 48000 * 2; // 1 second buffer
        std::vector<float> sample_buffer(buffer_size);
        
        int samples_processed = 0;
        while (samples_processed < total_samples && !cancelled) {
            const int samples_to_read = std::min(buffer_size, total_samples - samples_processed);
            
            if (!audio_source->get_audio_samples(samples_processed, samples_to_read, sample_buffer.data())) {
                m_last_error = "Failed to get audio samples at " + std::to_string(samples_processed);
                return false;
            }
            
            if (!encode_audio_samples(sample_buffer.data(), samples_to_read)) {
                m_last_error = "Failed to encode audio samples";
                return false;
            }
            
            samples_processed += samples_to_read;
            progress = static_cast<float>(samples_processed) / total_samples;
        }
        
        return finalize_audio_pipeline() && !cancelled;
    }
    
    bool export_av(FrameProvider* video_source, AudioProvider* audio_source) {
        m_last_error.clear();
        if (!video_source) {
            m_last_error = "video source is null";
            return false;
        }
        if (!initialize_av_pipeline()) {
            return false;
        }
        
        const int total_frames = video_source->get_total_frames();
        std::vector<uint8_t> video_buffer(config_.width * config_.height * 4);
        
        const int audio_total_samples = audio_source ? audio_source->get_total_samples() : 0;
        const int audio_buffer_size = 48000 * 2;
        std::vector<float> audio_buffer(audio_buffer_size);
        int audio_samples_processed = 0;
        
        for (int i = 0; i < total_frames && !cancelled; ++i) {
            // Video frame
            if (!video_source->get_frame(i, video_buffer.data(), config_.width * 4)) {
                m_last_error = "Failed to get frame " + std::to_string(i);
                return false;
            }
            
            if (!encode_video_frame(video_buffer.data())) {
                m_last_error = "Failed to encode video frame " + std::to_string(i);
                return false;
            }
            
            // Interleaved audio for this frame
            if (audio_source) {
                double frame_duration = 1.0 / video_source->get_fps();
                int samples_per_frame = static_cast<int>(frame_duration * audio_source->get_sample_rate());
                int samples_to_read = std::min(samples_per_frame, audio_total_samples - audio_samples_processed);
                
                if (samples_to_read > 0) {
                    if (!audio_source->get_audio_samples(audio_samples_processed, samples_to_read, audio_buffer.data())) {
                        m_last_error = "Failed to get audio samples";
                        return false;
                    } else if (!encode_audio_samples(audio_buffer.data(), samples_to_read)) {
                        m_last_error = "Failed to encode audio samples";
                        return false;
                    }
                    audio_samples_processed += samples_to_read;
                }
            }
            
            progress = static_cast<float>(i) / total_frames;
        }
        
        return finalize_av_pipeline() && !cancelled;
    }
    
    void cancel() { cancelled = true; }
    float get_progress() const { return progress.load(); }
    const std::string& last_error() const { return m_last_error; }
    
private:
    bool initialize_video_pipeline() {
        std::stringstream cmd;
        cmd << "ffmpeg -y -f rawvideo -pix_fmt rgba -s " << config_.width << "x" << config_.height
            << " -r " << config_.fps << " -i -";
        
        if (config_.use_crf) {
            cmd << " -crf " << config_.crf;
        } else {
            cmd << " -b:v " << config_.bitrate_kbps << "k";
        }
        
        cmd << " -c:v " << config_.video_codec
            << " -preset " << config_.preset
            << " -pix_fmt " << config_.pixel_format;
        
        if (config_.use_gpu) {
            cmd << " -hwaccel " << config_.gpu_device;
        }
        
        cmd << " -an \"" << output_path << "\"";
        
        video_pipe = _popen(cmd.str().c_str(), "w");
        return video_pipe != nullptr;
    }
    
    bool initialize_audio_pipeline() {
        std::stringstream cmd;
        cmd << "ffmpeg -y -f f32le -ar " << config_.sample_rate
            << " -ac " << config_.audio_channels
            << " -i -";
        
        cmd << " -c:a " << config_.audio_codec
            << " -b:a " << config_.audio_bitrate_kbps << "k";
        
        cmd << " -vn \"" << output_path << "\"";
        
        audio_pipe = _popen(cmd.str().c_str(), "w");
        return audio_pipe != nullptr;
    }
    
    bool initialize_av_pipeline() {
        std::stringstream cmd;
        cmd << "ffmpeg -y";
        
        // Video input
        cmd << " -f rawvideo -pix_fmt rgba -s " << config_.width << "x" << config_.height
            << " -r " << config_.fps << " -i -";
        
        // Audio input
        cmd << " -f f32le -ar " << config_.sample_rate
            << " -ac " << config_.audio_channels << " -i -";
        
        // Video encoding
        if (config_.use_crf) {
            cmd << " -crf " << config_.crf;
        } else {
            cmd << " -b:v " << config_.bitrate_kbps << "k";
        }
        cmd << " -c:v " << config_.video_codec << " -preset " << config_.preset;
        
        // Audio encoding
        cmd << " -c:a " << config_.audio_codec << " -b:a " << config_.audio_bitrate_kbps << "k";
        
        cmd << " \"" << output_path << "\"";
        
        av_pipe = _popen(cmd.str().c_str(), "w");
        return av_pipe != nullptr;
    }
    
    bool encode_video_frame(const uint8_t* rgba_data) {
        if (!video_pipe && !av_pipe) return false;
        FILE* pipe = video_pipe ? video_pipe : av_pipe;
        
        size_t frame_size = config_.width * config_.height * 4;
        size_t written = fwrite(rgba_data, 1, frame_size, pipe);
        return written == frame_size;
    }
    
    bool encode_audio_samples(const float* samples, int num_samples) {
        if (!audio_pipe && !av_pipe) return false;
        FILE* pipe = audio_pipe ? audio_pipe : av_pipe;
        
        size_t byte_size = num_samples * sizeof(float);
        size_t written = fwrite(samples, 1, byte_size, pipe);
        return written == byte_size;
    }
    
    bool finalize_video_pipeline() {
        if (video_pipe) {
            int result = _pclose(video_pipe);
            video_pipe = nullptr;
            return result == 0;
        }
        return true;
    }
    
    bool finalize_audio_pipeline() {
        if (audio_pipe) {
            int result = _pclose(audio_pipe);
            audio_pipe = nullptr;
            return result == 0;
        }
        return true;
    }
    
    bool finalize_av_pipeline() {
        if (av_pipe) {
            int result = _pclose(av_pipe);
            av_pipe = nullptr;
            return result == 0;
        }
        return true;
    }
    
    AVExportConfig config_;
    std::string output_path;
    std::atomic<bool> cancelled;
    std::atomic<float> progress;
    
    FILE* video_pipe = nullptr;
    FILE* audio_pipe = nullptr;
    FILE* av_pipe = nullptr;
};

AVExporter::AVExporter(const AVExportConfig& config) 
    : impl_(std::make_unique<Impl>(config)), config_(config) {}

AVExporter::~AVExporter() = default;

bool AVExporter::initialize(const std::string& output_path) {
    return impl_->initialize(output_path);
}

bool AVExporter::export_video(FrameProvider* video_source) {
    return impl_->export_video(video_source);
}

bool AVExporter::export_audio(AudioProvider* audio_source) {
    return impl_->export_audio(audio_source);
}

bool AVExporter::export_av(FrameProvider* video_source, AudioProvider* audio_source) {
    return impl_->export_av(video_source, audio_source);
}

void AVExporter::cancel() { impl_->cancel(); }
float AVExporter::get_progress() const { return impl_->get_progress(); }
const std::string& AVExporter::last_error() const { return impl_->last_error(); }

std::vector<std::string> get_available_video_codecs() {
    return {"libx264", "libx265", "prores_ks", "vp9", "mpeg4", "h264_nvenc", "hevc_nvenc"};
}

std::vector<std::string> get_available_audio_codecs() {
    return {"aac", "libmp3lame", "flac", "pcm_s16le", "opus"};
}

std::vector<std::string> get_available_containers() {
    return {"mp4", "mov", "mkv", "webm", "avi"};
}

bool check_ffmpeg_available() {
    FILE* pipe = _popen("ffmpeg -version", "r");
    if (!pipe) return false;
    _pclose(pipe);
    return true;
}

std::string get_ffmpeg_version() {
    FILE* pipe = _popen("ffmpeg -version 2>&1", "r");
    if (!pipe) return "Unknown";
    
    char buffer[256];
    std::string version = fgets(buffer, sizeof(buffer), pipe) ? buffer : "Unknown";
    _pclose(pipe);
    return version;
}

} // namespace tachyon
