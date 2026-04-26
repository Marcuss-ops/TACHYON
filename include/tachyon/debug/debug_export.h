#pragma once

#include <string>
#include <vector>
#include <cstdint>

namespace tachyon::debug {

/**
 * @brief Exports a RGBA surface to a PNG file for visual debugging.
 * 
 * @param path Output file path (should end in .png)
 * @param data Pointer to RGBA pixel data (32-bit per pixel)
 * @param width Width in pixels
 * @param height Height in pixels
 * @return true if export succeeded
 */
bool export_rgba_to_png(const std::string& path, const uint8_t* data, int width, int height);

/**
 * @brief Exports a float buffer as a grayscale PNG (for AOVs like depth, normals, etc.)
 * 
 * @param path Output file path (should end in .png)
 * @param data Pointer to float data
 * @param width Width in pixels
 * @param height Height in pixels
 * @param channel_name Name of the channel (for metadata)
 * @return true if export succeeded
 */
bool export_float_buffer_to_png(const std::string& path, const float* data, int width, int height, const std::string& channel_name = "");

/**
 * @brief Exports pipeline state as JSON for debugging.
 * 
 * @param path Output file path (should end in .json)
 * @param json_string The JSON content to write
 * @return true if export succeeded
 */
bool export_json(const std::string& path, const std::string& json_string);

/**
 * @brief Creates a debug snapshot directory if it doesn't exist.
 * 
 * @param base_path Base directory for snapshots (default: "debug_snapshots")
 * @return Full path to snapshot directory
 */
std::string ensure_snapshot_dir(const std::string& base_path = "debug_snapshots");

} // namespace tachyon::debug
