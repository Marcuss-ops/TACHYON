#include "tachyon/audio/audio_filter_builder.h"
#include <sstream>
#include <iomanip>

namespace tachyon::audio {

std::string AudioFilterBuilder::build_amix_filter(const AmixConfig& config) {
    std::stringstream ss;
    ss << std::fixed << std::setprecision(3);
    
    if (config.offset_seconds > 0) {
        ss << "atrim=start=" << config.offset_seconds << ",asetpts=PTS-STARTPTS,";
    }
    
    if (config.volume_db != 0.0) {
        ss << "volume=" << config.volume_db << "dB,";
    }
    
    ss << "amix=inputs=" << config.inputs << ":duration=longest";
    return ss.str();
}

std::string AudioFilterBuilder::build_background_music_filter(
    double duration,
    double fade_in_seconds,
    double fade_out_seconds,
    double volume_db) 
{
    std::stringstream ss;
    ss << std::fixed << std::setprecision(3);
    
    ss << "atrim=duration=" << duration << ",";
    
    if (fade_in_seconds > 0) {
        ss << "afade=t=in:st=0:d=" << fade_in_seconds << ",";
    }
    
    if (fade_out_seconds > 0) {
        ss << "afade=t=out:st=" << (duration - fade_out_seconds) << ":d=" << fade_out_seconds << ",";
    }
    
    if (volume_db != 0.0) {
        ss << "volume=" << volume_db << "dB";
    } else {
        // Remove trailing comma if no volume is added
        std::string s = ss.str();
        if (!s.empty() && s.back() == ',') s.pop_back();
        return s;
    }
    
    return ss.str();
}

std::string AudioFilterBuilder::build_audio_delay_filter(double delay_seconds) {
    std::stringstream ss;
    // adelay takes milliseconds
    long ms = static_cast<long>(delay_seconds * 1000.0);
    ss << "adelay=" << ms << "|" << ms;
    return ss.str();
}

std::string AudioFilterBuilder::build_acrossfade_filter(double duration_seconds) {
    std::stringstream ss;
    ss << std::fixed << std::setprecision(3);
    ss << "acrossfade=d=" << duration_seconds << ":c1=tri:c2=tri";
    return ss.str();
}

std::string AudioFilterBuilder::build_concat_audio_filter(int input_count) {
    std::stringstream ss;
    for (int i = 0; i < input_count; ++i) {
        ss << "[" << i << ":a]";
    }
    ss << "concat=n=" << input_count << ":v=0:a=1";
    return ss.str();
}

} // namespace tachyon::audio
