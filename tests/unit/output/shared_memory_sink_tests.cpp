#include "test_utils.h"
#include "tachyon/output/frame_output_sink.h"
#include "tachyon/renderer2d/core/framebuffer.h"
#include "tachyon/runtime/execution/jobs/render_job.h"

#include <iostream>
#include <memory>
#include <string>
#include <cmath>
#include <cstring>

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

namespace {

struct SharedMemoryHeader {
    uint32_t magic;
    uint32_t version;
    uint32_t width;
    uint32_t height;
    uint32_t stride_bytes;
    int64_t current_frame;
    double time_seconds;
};

} // namespace

bool run_shared_memory_sink_tests() {
    std::cout << "[SHMSinkTests] Starting Shared Memory Output Sink unit tests...\n";
    using namespace tachyon;
    using namespace tachyon::output;
    using namespace tachyon::renderer2d;

    // 1. Setup RenderPlan
    RenderPlan plan;
    plan.composition.width = 16;
    plan.composition.height = 16;
    plan.output.destination.path = "shm://test_sink_unit_test";
    plan.output.profile.format = OutputFormat::SharedMemory;

    // 2. Create the Sink via factory
    auto sink = create_frame_output_sink(plan);
    if (!sink) {
        std::cerr << "[SHMSinkTests] FAIL: create_frame_output_sink returned null for SharedMemory format\n";
        return false;
    }

    // 3. Initialize Sink
    bool ok = sink->begin(plan);
    if (!ok) {
        std::cerr << "[SHMSinkTests] FAIL: sink->begin failed: " << sink->last_error() << "\n";
        return false;
    }

    // 4. Create and Write Mock Frame
    auto frame = std::make_shared<SurfaceRGBA>(16, 16);
    // Draw a custom cross pattern of specific colors
    frame->clear(Color{0.2f, 0.4f, 0.6f, 1.0f});

    OutputFramePacket packet;
    packet.frame_number = 42;
    packet.metadata.time_seconds = 1.75;
    packet.frame = frame;

    bool write_ok = sink->write_frame(packet);
    if (!write_ok) {
        std::cerr << "[SHMSinkTests] FAIL: sink->write_frame failed: " << sink->last_error() << "\n";
        return false;
    }

    // 5. Open and inspect the Shared Memory segment directly to verify zero-copy transmission
    std::string shm_name = "test_sink_unit_test";
    size_t shm_size = sizeof(SharedMemoryHeader) + (16 * 16 * 4);
    void* m_mapped_addr = nullptr;

#ifdef _WIN32
    std::wstring wname(shm_name.begin(), shm_name.end());
    HANDLE hMap = OpenFileMappingW(FILE_MAP_READ, FALSE, wname.c_str());
    if (!hMap) {
        std::cerr << "[SHMSinkTests] FAIL: Could not open Win32 file mapping: " << GetLastError() << "\n";
        return false;
    }
    m_mapped_addr = MapViewOfFile(hMap, FILE_MAP_READ, 0, 0, shm_size);
    CloseHandle(hMap);
#else
    if (shm_name[0] != '/') {
        shm_name = "/" + shm_name;
    }
    int shm_fd = shm_open(shm_name.c_str(), O_RDONLY, 0444);
    if (shm_fd == -1) {
        std::cerr << "[SHMSinkTests] FAIL: Could not shm_open the segment: " << strerror(errno) << "\n";
        return false;
    }
    m_mapped_addr = mmap(nullptr, shm_size, PROT_READ, MAP_SHARED, shm_fd, 0);
    ::close(shm_fd);
#endif

    if (!m_mapped_addr || m_mapped_addr == (void*)-1) {
        std::cerr << "[SHMSinkTests] FAIL: Memory mapping lookup failed\n";
        return false;
    }

    // Inspect header
    const auto* header = static_cast<const SharedMemoryHeader*>(m_mapped_addr);
    if (header->magic != 0x54414348) {
        std::cerr << "[SHMSinkTests] FAIL: Header magic mismatch: " << std::hex << header->magic << "\n";
        return false;
    }

    if (header->width != 16 || header->height != 16 || header->stride_bytes != 64) {
        std::cerr << "[SHMSinkTests] FAIL: Header dimensions incorrect: " 
                  << header->width << "x" << header->height << "\n";
        return false;
    }

    if (header->current_frame != 42 || std::abs(header->time_seconds - 1.75) > 1e-5) {
        std::cerr << "[SHMSinkTests] FAIL: Header frame info incorrect: "
                  << header->current_frame << ", time: " << header->time_seconds << "\n";
        return false;
    }

    // Inspect pixel bytes (RGBA8 format)
    const uint8_t* pixels = static_cast<const uint8_t*>(m_mapped_addr) + sizeof(SharedMemoryHeader);
    // r=0.2f -> 51, g=0.4f -> 102, b=0.6f -> 153, a=1.0f -> 255
    if (pixels[0] != 51 || pixels[1] != 102 || pixels[2] != 153 || pixels[3] != 255) {
        std::cerr << "[SHMSinkTests] FAIL: Shared memory pixel values incorrect: ("
                  << (int)pixels[0] << ", " << (int)pixels[1] << ", " << (int)pixels[2] << ")\n";
        return false;
    }

    // Unmap
#ifdef _WIN32
    UnmapViewOfFile(m_mapped_addr);
#else
    munmap(m_mapped_addr, shm_size);
#endif

    std::cout << "[SHMSinkTests] Shared memory content and atomic headers verified successfully!\n";

    // 6. Finish and Cleanup
    sink->finish();

    std::cout << "[SHMSinkTests] All Shared Memory Sink unit tests passed successfully!\n";
    return true;
}
