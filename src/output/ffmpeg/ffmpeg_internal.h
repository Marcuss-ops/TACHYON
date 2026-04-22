#pragma once
#include "tachyon/output/frame_output_sink.h"
#include "tachyon/renderer2d/color/color_transfer.h"
#include <string>
#include <filesystem>
#include <vector>

namespace tachyon::output {

std::string build_ffmpeg_video_command(const RenderPlan& plan, const std::filesystem::path& output_path, bool include_faststart);
std::string build_ffmpeg_mux_command(const RenderPlan& plan, const std::filesystem::path& video_path, const std::filesystem::path& audio_path);
std::filesystem::path make_ffmpeg_temp_path(const std::filesystem::path& destination, const std::string& suffix, const std::string& extension);

std::vector<unsigned char> convert_and_pack_ffmpeg_frame(
    const renderer2d::Framebuffer& frame,
    uint32_t target_width,
    uint32_t target_height,
    renderer2d::detail::TransferCurve source_curve,
    renderer2d::detail::ColorSpace source_space,
    renderer2d::detail::TransferCurve output_curve,
    renderer2d::detail::ColorSpace output_space,
    renderer2d::detail::ColorRange output_range);

} // namespace tachyon::output
