#include "tachyon/runtime/playback/playback_engine.h"
#include "tachyon/runtime/cache/disk_cache.h"

namespace tachyon::runtime {

PlaybackEngine::PlaybackEngine(DiskCacheStore *disk_cache)
    : m_disk_cache(disk_cache) {}

PlaybackEngine::~PlaybackEngine() { stop(); }

void PlaybackEngine::start() {
  if (m_running)
    return;
  m_running = true;
  m_worker = std::thread(&PlaybackEngine::worker_loop, this);
}

void PlaybackEngine::stop() {
  m_running = false;
  m_cv.notify_all();
  if (m_worker.joinable()) {
    m_worker.join();
  }
}

void PlaybackEngine::request_frame(const FrameRequest &req) {
  std::lock_guard<std::mutex> lock(m_mutex);
  m_queue.push(req);
  m_cv.notify_one();
}

std::shared_ptr<renderer2d::SurfaceRGBA>
PlaybackEngine::get_frame(int64_t frame_number) {
  std::lock_guard<std::mutex> lock(m_mutex);
  auto it = m_ram_cache.find(frame_number);
  if (it != m_ram_cache.end()) {
    return it->second;
  }

  if (m_disk_cache) {
    CacheKey key;
    key.identity = static_cast<uint64_t>(frame_number);
    key.node_path = "frame_render";
    key.frame_time = static_cast<double>(frame_number); // Placeholder
    
    auto data = m_disk_cache->load(key);
    if (data) {
      // In a real implementation, we would decode the binary data back to SurfaceRGBA
      // For now, this is a placeholder matching the previous logic.
      return nullptr;
    }
  }

  return nullptr;
}

void PlaybackEngine::set_current_position(int64_t frame_number) {
  int64_t prev_frame = m_current_frame;
  m_current_frame = frame_number;

  // 1. Request current frame with highest priority
  request_frame({frame_number, 0.0, 0});

  // 2. Predictive pre-fetch based on motion direction
  int direction = (frame_number >= prev_frame) ? 1 : -1;

  // Immediate neighbors (high priority)
  for (int i = 1; i <= 3; ++i) {
    request_frame({frame_number + i * direction, 0.0, 1});
  }

  // Distant future (lower priority)
  for (int i = 4; i <= 15; ++i) {
    request_frame({frame_number + i * direction, 0.0, 5});
  }

  // Immediate past (lowest priority, just in case of reversal)
  request_frame({frame_number - direction, 0.0, 10});

  // Prune RAM cache: keep only frames within +/- 30 of current
  std::lock_guard<std::mutex> lock(m_mutex);
  for (auto it = m_ram_cache.begin(); it != m_ram_cache.end();) {
    if (std::abs(it->first - m_current_frame) > 30) {
      it = m_ram_cache.erase(it);
    } else {
      ++it;
    }
  }
}

void PlaybackEngine::worker_loop() {
    // Worker logic placeholder
}

} // namespace tachyon::runtime
