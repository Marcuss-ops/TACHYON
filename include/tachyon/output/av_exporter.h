#pragma once

#include <string>
#include <vector>
#include <memory>
#include <optional>

namespace tachyon {

struct AVExportConfig {
    // Video settings
    std::string video_codec = "libx264";      // libx264, libx265, prores_ks, vp9
    std::string preset = "medium";             // ultrafast, superfast, medium, slow, veryslow
    int crf = 23;                            // 0-51 (lower = better quality)
    int bitrate_kbps = 8000;                 // Used if crf not set
    std::string pixel_format = "yuv420p";     // yuv420p, yuv422p, yuv444p
    bool use_crf = true;
    
    // Audio settings
    std::string audio_codec = "aac";         // aac, libmp3lame, flac
    int audio_bitrate_kbps = 192;
    int sample_rate = 48000;
    int audio_channels = 2;
    
    // Container
    std::string container = "mp4";          // mp4, mov, mkv, webm
    
    // Resolution & FPS
    int width = 1920;
    int height = 1080;
    double fps = 30.0;
    
    // Advanced
    bool use_gpu = false;                    // Hardware acceleration
    std::string gpu_device = "";             // cuda, vaapi, qsv, videotoolbox
    bool generate_proxies = false;          // Create proxy files during export
    std::string proxy_path = "";
};

class FrameProvider {
public:
    virtual ~FrameProvider() = default;
    virtual bool get_frame(int frame_index, uint8_t* rgba_data, int stride) = 0;
    virtual int get_total_frames() const = 0;
    virtual double get_fps() const = 0;
};

class AudioProvider {
public:
    virtual ~AudioProvider() = default;
    virtual bool get_audio_samples(int sample_offset, int num_samples, float* interleaved_buffer) = 0;
    virtual int get_total_samples() const = 0;
    virtual int get_sample_rate() const = 0;
    virtual int get_channels() const = 0;
};

class AVExporter {
public:
    explicit AVExporter(const AVExportConfig& config);
    ~AVExporter();
    
    // Non-copyable
    AVExporter(const AVExporter&) = delete;
    AVExporter& operator=(const AVExporter&) = delete;
    
    bool initialize(const std::string& output_path);
    bool export_video(FrameProvider* video_source);
    bool export_audio(AudioProvider* audio_source);
    bool export_av(FrameProvider* video_source, AudioProvider* audio_source);
    void cancel();
    float get_progress() const;
    [[nodiscard]] const std::string& last_error() const;
    
private:
    class Impl;
    std::unique_ptr<Impl> impl_;
    AVExportConfig config_;
};

// Utility functions
std::vector<std::string> get_available_video_codecs();
std::vector<std::string> get_available_audio_codecs();
std::vector<std::string> get_available_containers();
bool check_ffmpeg_available();
std::string get_ffmpeg_version();

} // namespace tachyon
