#pragma once

#include <atomic>
#include <thread>
#include <vector>
#include <cstddef>
#include <mutex>

namespace tachyon {

/**
 * @brief Background sampler for CPU and Memory usage of the current process.
 */
class ProcessResourceSampler {
public:
    ProcessResourceSampler(int interval_ms = 250);
    ~ProcessResourceSampler();

    void start();
    void stop();

    std::size_t avg_working_set_bytes() const;
    std::size_t peak_working_set_bytes() const;
    std::size_t avg_private_bytes() const;
    std::size_t peak_private_bytes() const;
    
    double avg_cpu_percent_machine() const; // 0-100%
    double avg_cpu_cores_used() const;      // 0-N

    double cpu_user_ms() const { return m_cpu_user_ms; }
    double cpu_kernel_ms() const { return m_cpu_kernel_ms; }

private:
    void sample_loop();
    
    // Platform specific resource fetching
    struct RawSample {
        std::size_t working_set_bytes;
        std::size_t private_bytes;
        double cpu_percent_machine;
        double cpu_cores_used;
    };
    RawSample capture_sample();

    int m_interval_ms;
    std::atomic<bool> m_running{false};
    std::thread m_thread;

    mutable std::mutex m_data_mutex;
    std::vector<std::size_t> m_working_set_samples;
    std::vector<std::size_t> m_private_bytes_samples;
    std::vector<double> m_cpu_machine_samples;
    std::vector<double> m_cpu_cores_samples;
    
    double m_cpu_user_ms{0.0};
    double m_cpu_kernel_ms{0.0};
    
    // State for CPU calculation
    uint64_t m_last_process_time{0};
    uint64_t m_last_system_time{0};
};

} // namespace tachyon
