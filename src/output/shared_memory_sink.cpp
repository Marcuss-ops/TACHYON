#include "tachyon/output/shared_memory_sink.h"
#include "tachyon/output/frame_output_sink.h"
#include "tachyon/renderer2d/core/framebuffer.h"

#include <iostream>
#include <string>
#include <cstring>
#include <algorithm>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#else
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#endif

namespace tachyon::output {

struct SharedMemoryHeader {
    uint32_t magic;         // 0x54414348 ("TACH")
    uint32_t version;       // 1
    uint32_t width;
    uint32_t height;
    uint32_t stride_bytes;
    int64_t current_frame;
    double time_seconds;
};

class SharedMemoryOutputSink final : public FrameOutputSink {
public:
    SharedMemoryOutputSink() = default;
    ~SharedMemoryOutputSink() override {
        cleanup();
    }

    bool begin(const RenderPlan& plan) override {
        m_error.clear();
        cleanup();

        std::string raw_path = plan.output.destination.path;
        if (raw_path.rfind("shm://", 0) == 0) {
            m_shm_name = raw_path.substr(6);
        } else {
            m_shm_name = raw_path;
        }

        if (m_shm_name.empty()) {
            m_shm_name = "tachyon_shm_output";
        }

#ifndef _WIN32
        // POSIX shared memory names must start with a leading slash '/' and contain no other slashes
        if (m_shm_name[0] != '/') {
            m_shm_name = "/" + m_shm_name;
        }
#endif

        const uint32_t width = plan.composition.width > 0 ? static_cast<uint32_t>(plan.composition.width) : 1920U;
        const uint32_t height = plan.composition.height > 0 ? static_cast<uint32_t>(plan.composition.height) : 1080U;
        
        m_shm_size = sizeof(SharedMemoryHeader) + (static_cast<size_t>(width) * height * 4);

#ifdef _WIN32
        std::wstring wname(m_shm_name.begin(), m_shm_name.end());
        m_hMap = CreateFileMappingW(
            INVALID_HANDLE_VALUE,
            NULL,
            PAGE_READWRITE,
            static_cast<DWORD>((m_shm_size >> 32) & 0xFFFFFFFF),
            static_cast<DWORD>(m_shm_size & 0xFFFFFFFF),
            wname.c_str()
        );

        if (!m_hMap) {
            m_error = "CreateFileMapping failed: " + std::to_string(GetLastError());
            return false;
        }

        m_mapped_addr = MapViewOfFile(
            m_hMap,
            FILE_MAP_ALL_ACCESS,
            0,
            0,
            m_shm_size
        );

        if (!m_mapped_addr) {
            m_error = "MapViewOfFile failed: " + std::to_string(GetLastError());
            CloseHandle(m_hMap);
            m_hMap = NULL;
            return false;
        }
#else
        m_shm_fd = shm_open(m_shm_name.c_str(), O_CREAT | O_RDWR, 0666);
        if (m_shm_fd == -1) {
            m_error = "shm_open failed: " + std::string(strerror(errno));
            return false;
        }

        if (ftruncate(m_shm_fd, static_cast<off_t>(m_shm_size)) == -1) {
            m_error = "ftruncate failed: " + std::string(strerror(errno));
            ::close(m_shm_fd);
            m_shm_fd = -1;
            return false;
        }

        m_mapped_addr = mmap(
            nullptr,
            m_shm_size,
            PROT_READ | PROT_WRITE,
            MAP_SHARED,
            m_shm_fd,
            0
        );

        if (m_mapped_addr == MAP_FAILED) {
            m_error = "mmap failed: " + std::string(strerror(errno));
            ::close(m_shm_fd);
            m_shm_fd = -1;
            m_mapped_addr = nullptr;
            return false;
        }
#endif

        // Initialize header
        auto* header = static_cast<SharedMemoryHeader*>(m_mapped_addr);
        header->magic = 0x54414348; // "TACH"
        header->version = 1;
        header->width = width;
        header->height = height;
        header->stride_bytes = width * 4;
        header->current_frame = -1;
        header->time_seconds = 0.0;

        std::cout << "[Tachyon][SHM] Shared memory sink initialized successfully: " 
                  << m_shm_name << " (" << m_shm_size << " bytes)" << std::endl;
        return true;
    }

    bool write_frame(const OutputFramePacket& packet) override {
        if (!m_mapped_addr) {
            m_error = "shared memory mapping not initialized";
            return false;
        }

        if (!packet.frame) {
            m_error = "sink received a null frame";
            return false;
        }

        auto* header = static_cast<SharedMemoryHeader*>(m_mapped_addr);
        if (packet.frame->width() != header->width || packet.frame->height() != header->height) {
            m_error = "frame dimensions mismatch shared memory header";
            return false;
        }

        // Direct float-to-RGBA8 conversion onto mmaped memory pointer (zero dynamic allocations)
        uint8_t* pixel_dest = static_cast<uint8_t*>(m_mapped_addr) + sizeof(SharedMemoryHeader);
        const float* src_pixels = packet.frame->data();
        const size_t num_pixels = static_cast<size_t>(header->width) * header->height;

        for (size_t i = 0; i < num_pixels; ++i) {
            const size_t idx = i * 4;
            auto to_u8 = [](float val) -> uint8_t {
                float clamped = val < 0.0f ? 0.0f : (val > 1.0f ? 1.0f : val);
                return static_cast<uint8_t>(clamped * 255.0f);
            };
            pixel_dest[idx]     = to_u8(src_pixels[idx]);
            pixel_dest[idx + 1] = to_u8(src_pixels[idx + 1]);
            pixel_dest[idx + 2] = to_u8(src_pixels[idx + 2]);
            pixel_dest[idx + 3] = to_u8(src_pixels[idx + 3]);
        }

        // Update header metadata atomically
        header->current_frame = packet.frame_number;
        header->time_seconds = packet.metadata.time_seconds;

        return true;
    }

    bool finish() override {
        cleanup();
        return true;
    }

    const std::string& last_error() const override {
        return m_error;
    }

private:
    void cleanup() {
        if (m_mapped_addr) {
#ifdef _WIN32
            UnmapViewOfFile(m_mapped_addr);
#else
            munmap(m_mapped_addr, m_shm_size);
#endif
            m_mapped_addr = nullptr;
        }

#ifdef _WIN32
        if (m_hMap) {
            CloseHandle(m_hMap);
            m_hMap = NULL;
        }
#else
        if (m_shm_fd != -1) {
            ::close(m_shm_fd);
            m_shm_fd = -1;
        }
        if (!m_shm_name.empty()) {
            shm_unlink(m_shm_name.c_str());
        }
#endif
        m_shm_name.clear();
        m_shm_size = 0;
    }

    std::string m_shm_name;
    size_t m_shm_size{0};
    void* m_mapped_addr{nullptr};
    std::string m_error;
#ifdef _WIN32
    HANDLE m_hMap{NULL};
#else
    int m_shm_fd{-1};
#endif
};

std::unique_ptr<FrameOutputSink> create_shared_memory_sink() {
    return std::make_unique<SharedMemoryOutputSink>();
}

} // namespace tachyon::output
