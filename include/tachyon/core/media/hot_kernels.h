#pragma once

#include <vector>
#include <cstddef>

namespace tachyon::audio {

/**
 * @brief High-performance audio processing kernels.
 * Derived from ruststream-core/audio/hot_kernels.rs
 * 
 * These kernels are designed for maximum throughput, utilizing SIMD
 * paths where available and avoiding unnecessary allocations.
 */
class HotKernels {
public:
    /**
     * @brief Mixes multiple input buffers into an output buffer with clipping.
     * @param inputs Vector of pointers to input float buffers.
     * @param output Pointer to the output float buffer.
     * @param frames Number of samples to process.
     */
    static void audio_mix(
        const std::vector<const float*>& inputs,
        float* output,
        size_t frames);

    /**
     * @brief Applies gain to a buffer.
     * @param buffer Pointer to the input/output float buffer.
     * @param frames Number of samples to process.
     * @param volume Linear volume multiplier.
     */
    static void apply_volume(
        float* buffer,
        size_t frames,
        float volume);

    /**
     * @brief Zeros out a range in a buffer (gating).
     * @param buffer Pointer to the float buffer.
     * @param frames Total number of samples in the buffer.
     * @param start_frame Index of the first frame to zero out.
     * @param end_frame Index of the last frame to zero out (exclusive).
     */
    static void apply_gate(
        float* buffer,
        size_t frames,
        size_t start_frame,
        size_t end_frame);
};

/**
 * @brief Thread-local buffer pool for temporary audio processing buffers.
 * Avoids repeated allocations in the hot path.
 */
class AudioBufferPool {
public:
    static float* acquire(size_t frames);
    static void release(float* buffer);
};

} // namespace tachyon::audio
