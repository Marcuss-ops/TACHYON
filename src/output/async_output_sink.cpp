#include "tachyon/output/frame_output_sink.h"
#include <queue>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <atomic>
#include <iostream>

namespace tachyon::output {

namespace {

class AsyncFrameOutputSink final : public FrameOutputSink {
public:
    explicit AsyncFrameOutputSink(std::unique_ptr<FrameOutputSink> inner, size_t max_queue_size = 8)
        : m_inner(std::move(inner)), m_max_queue_size(max_queue_size) {
    }

    ~AsyncFrameOutputSink() override {
        if (m_running) {
            finish();
        }
    }

    bool begin(const RenderPlan& plan) override {
        if (!m_inner->begin(plan)) {
            return false;
        }
        
        m_running = true;
        m_worker = std::thread([this]() {
            worker_loop();
        });
        
        return true;
    }

    bool write_frame(const OutputFramePacket& packet) override {
        if (!m_running || m_has_error) return false;

        // Clone the frame immediately on the render thread
        // This ensures the data is safe even if the render thread proceeds
        auto owned_frame = std::make_unique<renderer2d::Framebuffer>(*packet.frame);
        
        {
            std::unique_lock<std::mutex> lock(m_mutex);
            // Bounded queue: block the render thread if we are too far ahead
            // This prevents memory exhaustion
            m_cv_can_push.wait(lock, [this]() { return m_queue.size() < m_max_queue_size || !m_running; });
            
            if (!m_running) return false;

            QueuedPacket qp;
            qp.frame = std::move(owned_frame);
            qp.metadata = packet.metadata;
            qp.frame_number = packet.frame_number;
            // Note: AOVs are not cloned yet for simplicity
            
            m_queue.push(std::move(qp));
        }
        
        m_cv_can_pop.notify_one();
        return true;
    }

    bool finish() override {
        if (m_running) {
            {
                std::lock_guard<std::mutex> lock(m_mutex);
                m_running = false;
            }
            m_cv_can_pop.notify_all();
            m_cv_can_push.notify_all();
            
            if (m_worker.joinable()) {
                m_worker.join();
            }
        }
        return m_inner->finish();
    }

    const std::string& last_error() const override {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (!m_error_msg.empty()) return m_error_msg;
        return m_inner->last_error();
    }

private:
    struct QueuedPacket {
        std::unique_ptr<renderer2d::Framebuffer> frame;
        FrameMetadata metadata;
        std::int64_t frame_number;
    };

    void worker_loop() {
        while (true) {
            QueuedPacket qp;
            {
                std::unique_lock<std::mutex> lock(m_mutex);
                m_cv_can_pop.wait(lock, [this]() { return !m_queue.empty() || !m_running; });
                
                if (m_queue.empty() && !m_running) break;
                
                qp = std::move(m_queue.front());
                m_queue.pop();
            }
            m_cv_can_push.notify_one();

            OutputFramePacket packet;
            packet.frame = qp.frame.get();
            packet.metadata = qp.metadata;
            packet.frame_number = qp.frame_number;

            if (!m_inner->write_frame(packet)) {
                std::lock_guard<std::mutex> lock(m_mutex);
                m_has_error = true;
                m_error_msg = m_inner->last_error();
                m_running = false;
                break;
            }
        }
    }

    std::unique_ptr<FrameOutputSink> m_inner;
    size_t m_max_queue_size;
    std::queue<QueuedPacket> m_queue;
    mutable std::mutex m_mutex;
    std::condition_variable m_cv_can_push;
    std::condition_variable m_cv_can_pop;
    std::thread m_worker;
    std::atomic<bool> m_running{false};
    std::atomic<bool> m_has_error{false};
    std::string m_error_msg;
};

} // namespace

std::unique_ptr<FrameOutputSink> create_async_output_sink(std::unique_ptr<FrameOutputSink> inner) {
    if (!inner) return nullptr;
    return std::make_unique<AsyncFrameOutputSink>(std::move(inner));
}

} // namespace tachyon::output
