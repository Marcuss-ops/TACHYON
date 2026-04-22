#pragma once
#include <vector>
#include <memory>
#include <string>
#include <functional>

namespace tachyon::audio {

static constexpr int kDSPSampleRate = 48000;
static constexpr int kDSPChannels   = 2;

// ---------------------------------------------------------------------------
// AudioNode – base class for all DSP processing nodes
// ---------------------------------------------------------------------------
class AudioNode {
public:
    virtual ~AudioNode() = default;
    // Process in-place: interleaved stereo float, frames = nframes * 2 samples
    virtual void process(float* io, int nframes) = 0;
    virtual void reset() {}
    virtual std::string name() const = 0;
};

// ---------------------------------------------------------------------------
// AudioBus – a chain of nodes processed in order
// ---------------------------------------------------------------------------
class AudioBus {
public:
    explicit AudioBus(std::string id = "") : m_id(std::move(id)) {}

    void add_node(std::shared_ptr<AudioNode> node) { m_nodes.push_back(std::move(node)); }
    void clear_nodes() { m_nodes.clear(); }

    void process(float* io, int nframes) {
        for (auto& node : m_nodes) node->process(io, nframes);
    }

    void reset() { for (auto& n : m_nodes) n->reset(); }
    const std::string& id() const { return m_id; }

private:
    std::string m_id;
    std::vector<std::shared_ptr<AudioNode>> m_nodes;
};

// ---------------------------------------------------------------------------
// AudioGraph – collection of buses; each track goes through its bus then master
// ---------------------------------------------------------------------------
class AudioGraph {
public:
    AudioBus& get_or_create_bus(const std::string& id) {
        for (auto& b : m_buses) if (b.id() == id) return b;
        m_buses.emplace_back(id);
        return m_buses.back();
    }

    AudioBus& master() { return m_master; }

    // Process a buffer: through per-track bus then master chain
    void process_track(const std::string& bus_id, float* io, int nframes) {
        for (auto& b : m_buses) {
            if (b.id() == bus_id) { b.process(io, nframes); break; }
        }
        m_master.process(io, nframes);
    }

    void reset_all() {
        for (auto& b : m_buses) b.reset();
        m_master.reset();
    }

private:
    std::vector<AudioBus> m_buses;
    AudioBus              m_master{"master"};
};

} // namespace tachyon::audio
