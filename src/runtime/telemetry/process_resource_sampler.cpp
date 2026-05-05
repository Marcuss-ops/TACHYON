#include "tachyon/runtime/telemetry/process_resource_sampler.h"

#ifdef _WIN32
#include <windows.h>
#include <psapi.h>
#endif

#include <numeric>
#include <algorithm>
#include <chrono>

namespace tachyon {

ProcessResourceSampler::ProcessResourceSampler(int interval_ms)
    : m_interval_ms(interval_ms) {}

ProcessResourceSampler::~ProcessResourceSampler() {
    stop();
}

void ProcessResourceSampler::start() {
    if (m_running.exchange(true)) return;
    
    // Initialize CPU tracking
#ifdef _WIN32
    FILETIME creation_time, exit_time, kernel_time, user_time;
    if (GetProcessTimes(GetCurrentProcess(), &creation_time, &exit_time, &kernel_time, &user_time)) {
        ULARGE_INTEGER kernel, user;
        kernel.LowPart = kernel_time.dwLowDateTime;
        kernel.HighPart = kernel_time.dwHighDateTime;
        user.LowPart = user_time.dwLowDateTime;
        user.HighPart = user_time.dwHighDateTime;
        
        m_last_process_time = kernel.QuadPart + user.QuadPart;
    }
    
    FILETIME sys_time;
    GetSystemTimeAsFileTime(&sys_time);
    ULARGE_INTEGER sys;
    sys.LowPart = sys_time.dwLowDateTime;
    sys.HighPart = sys_time.dwHighDateTime;
    m_last_system_time = sys.QuadPart;
#endif

    m_thread = std::thread(&ProcessResourceSampler::sample_loop, this);
}

void ProcessResourceSampler::stop() {
    if (!m_running.exchange(false)) return;
    if (m_thread.joinable()) {
        m_thread.join();
    }
}

void ProcessResourceSampler::sample_loop() {
    while (m_running) {
        auto sample = capture_sample();
        
        {
            std::lock_guard<std::mutex> lock(m_data_mutex);
            m_working_set_samples.push_back(sample.working_set_bytes);
            m_private_bytes_samples.push_back(sample.private_bytes);
            m_cpu_machine_samples.push_back(sample.cpu_percent_machine);
            m_cpu_cores_samples.push_back(sample.cpu_cores_used);
        }
        
        std::this_thread::sleep_for(std::chrono::milliseconds(m_interval_ms));
    }
}

ProcessResourceSampler::RawSample ProcessResourceSampler::capture_sample() {
    RawSample sample{0, 0, 0.0, 0.0};
    
#ifdef _WIN32
    PROCESS_MEMORY_COUNTERS_EX pmc;
    if (GetProcessMemoryInfo(GetCurrentProcess(), (PROCESS_MEMORY_COUNTERS*)&pmc, sizeof(pmc))) {
        sample.working_set_bytes = pmc.WorkingSetSize;
        sample.private_bytes = pmc.PrivateUsage;
    }
    
    FILETIME creation_time, exit_time, kernel_time, user_time;
    if (GetProcessTimes(GetCurrentProcess(), &creation_time, &exit_time, &kernel_time, &user_time)) {
        ULARGE_INTEGER kernel, user;
        kernel.LowPart = kernel_time.dwLowDateTime;
        kernel.HighPart = kernel_time.dwHighDateTime;
        user.LowPart = user_time.dwLowDateTime;
        user.HighPart = user_time.dwHighDateTime;
        
        m_cpu_kernel_ms = static_cast<double>(kernel.QuadPart) / 10000.0;
        m_cpu_user_ms = static_cast<double>(user.QuadPart) / 10000.0;
        
        uint64_t now_process_time = kernel.QuadPart + user.QuadPart;
        
        FILETIME sys_time;
        GetSystemTimeAsFileTime(&sys_time);
        ULARGE_INTEGER sys;
        sys.LowPart = sys_time.dwLowDateTime;
        sys.HighPart = sys_time.dwHighDateTime;
        uint64_t now_system_time = sys.QuadPart;
        
        if (now_system_time > m_last_system_time) {
            uint64_t process_delta = now_process_time - m_last_process_time;
            uint64_t system_delta = now_system_time - m_last_system_time;
            
            if (system_delta > 0) {
                double cores_used = static_cast<double>(process_delta) / static_cast<double>(system_delta);
                sample.cpu_cores_used = cores_used;
                
                static int num_cores = []() {
                    SYSTEM_INFO si;
                    GetSystemInfo(&si);
                    return static_cast<int>(si.dwNumberOfProcessors);
                }();
                
                sample.cpu_percent_machine = (cores_used / static_cast<double>(num_cores)) * 100.0;
            }
        }
        
        m_last_process_time = now_process_time;
        m_last_system_time = now_system_time;
    }
#endif

    return sample;
}

std::size_t ProcessResourceSampler::avg_working_set_bytes() const {
    std::lock_guard<std::mutex> lock(m_data_mutex);
    if (m_working_set_samples.empty()) return 0;
    return std::accumulate(m_working_set_samples.begin(), m_working_set_samples.end(), std::size_t(0)) / m_working_set_samples.size();
}

std::size_t ProcessResourceSampler::peak_working_set_bytes() const {
    std::lock_guard<std::mutex> lock(m_data_mutex);
    if (m_working_set_samples.empty()) return 0;
    return *std::max_element(m_working_set_samples.begin(), m_working_set_samples.end());
}

std::size_t ProcessResourceSampler::avg_private_bytes() const {
    std::lock_guard<std::mutex> lock(m_data_mutex);
    if (m_private_bytes_samples.empty()) return 0;
    return std::accumulate(m_private_bytes_samples.begin(), m_private_bytes_samples.end(), std::size_t(0)) / m_private_bytes_samples.size();
}

std::size_t ProcessResourceSampler::peak_private_bytes() const {
    std::lock_guard<std::mutex> lock(m_data_mutex);
    if (m_private_bytes_samples.empty()) return 0;
    return *std::max_element(m_private_bytes_samples.begin(), m_private_bytes_samples.end());
}

double ProcessResourceSampler::avg_cpu_percent_machine() const {
    std::lock_guard<std::mutex> lock(m_data_mutex);
    if (m_cpu_machine_samples.empty()) return 0.0;
    return std::accumulate(m_cpu_machine_samples.begin(), m_cpu_machine_samples.end(), 0.0) / m_cpu_machine_samples.size();
}

double ProcessResourceSampler::avg_cpu_cores_used() const {
    std::lock_guard<std::mutex> lock(m_data_mutex);
    if (m_cpu_cores_samples.empty()) return 0.0;
    return std::accumulate(m_cpu_cores_samples.begin(), m_cpu_cores_samples.end(), 0.0) / m_cpu_cores_samples.size();
}

} // namespace tachyon
