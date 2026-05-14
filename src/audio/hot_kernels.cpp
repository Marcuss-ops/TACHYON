#include "tachyon/core/media/hot_kernels.h"
#include <algorithm>
#include <cstring>
#include <vector>

namespace tachyon::audio {

void HotKernels::audio_mix(
    const std::vector<const float*>& inputs,
    float* output,
    size_t frames) 
{
    if (frames == 0 || !output) return;

    if (inputs.empty()) {
        std::memset(output, 0, frames * sizeof(float));
        return;
    }

    // Initialize output with first input
    std::memcpy(output, inputs[0], frames * sizeof(float));

    // Mix remaining inputs
    for (size_t i = 1; i < inputs.size(); ++i) {
        const float* input = inputs[i];
        if (!input) continue;
        
        for (size_t f = 0; f < frames; ++f) {
            output[f] += input[f];
        }
    }

    // Clip output to [-1.0, 1.0] to prevent digital distortion
    for (size_t f = 0; f < frames; ++f) {
        if (output[f] > 1.0f) output[f] = 1.0f;
        else if (output[f] < -1.0f) output[f] = -1.0f;
    }
}

void HotKernels::apply_volume(float* buffer, size_t frames, float volume) {
    if (volume == 1.0f || !buffer || frames == 0) return;
    
    if (volume == 0.0f) {
        std::memset(buffer, 0, frames * sizeof(float));
        return;
    }

    for (size_t f = 0; f < frames; ++f) {
        buffer[f] *= volume;
    }
}

void HotKernels::apply_gate(float* buffer, size_t frames, size_t start_frame, size_t end_frame) {
    if (!buffer || frames == 0) return;
    
    const size_t actual_start = std::min(start_frame, frames);
    const size_t actual_end = std::min(end_frame, frames);
    
    if (actual_start >= actual_end) return;

    std::memset(buffer + actual_start, 0, (actual_end - actual_start) * sizeof(float));
}

/**
 * @brief Simple thread-local buffer implementation for the pool.
 * A production implementation would use a more sophisticated allocation strategy.
 */
struct ThreadBuffer {
    std::vector<float> data;
    bool in_use{false};
};

static thread_local std::vector<ThreadBuffer> tl_buffers;

float* AudioBufferPool::acquire(size_t frames) {
    for (auto& buf : tl_buffers) {
        if (!buf.in_use && buf.data.size() >= frames) {
            buf.in_use = true;
            return buf.data.data();
        }
    }
    
    tl_buffers.push_back({std::vector<float>(frames), true});
    return tl_buffers.back().data.data();
}

void AudioBufferPool::release(float* buffer) {
    for (auto& buf : tl_buffers) {
        if (buf.data.data() == buffer) {
            buf.in_use = false;
            return;
        }
    }
}

} // namespace tachyon::audio
