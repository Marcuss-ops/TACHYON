#pragma once

#include "tachyon/renderer2d/core/framebuffer.h"

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <mutex>
#include <unordered_map>
#include <vector>

namespace tachyon {

class SurfacePoolFast;

// One allocation per surface; stays alive until SurfacePoolFast::clear().
struct alignas(64) SurfaceBlock {
    std::atomic<uint32_t> refcount{0};
    SurfacePoolFast* owner{nullptr};
    SurfaceBlock* next{nullptr};
    renderer2d::SurfaceRGBA surface;

    SurfaceBlock(uint32_t w, uint32_t h) : surface(w, h) {}
};

// Lightweight handle — intrusive refcount, no control-block allocation.
class SurfaceRef {
public:
    SurfaceRef() = default;
    ~SurfaceRef() { release(); }

    SurfaceRef(const SurfaceRef& o) : block_(o.block_) {
        if (block_) block_->refcount.fetch_add(1, std::memory_order_relaxed);
    }
    SurfaceRef& operator=(const SurfaceRef& o);

    SurfaceRef(SurfaceRef&& o) noexcept : block_(o.block_) { o.block_ = nullptr; }
    SurfaceRef& operator=(SurfaceRef&& o) noexcept;

    renderer2d::SurfaceRGBA* get()       const noexcept { return block_ ? &block_->surface : nullptr; }
    renderer2d::SurfaceRGBA& operator*() const noexcept { return block_->surface; }
    renderer2d::SurfaceRGBA* operator->()const noexcept { return &block_->surface; }
    explicit operator bool()             const noexcept { return block_ != nullptr; }

private:
    friend class SurfacePoolFast;
    // Pool sets refcount=1 before handing out; constructor does not addref.
    explicit SurfaceRef(SurfaceBlock* b) noexcept : block_(b) {}
    void release();

    SurfaceBlock* block_{nullptr};
};

// Faster drop-in for SurfacePool.
// Contract: pool must outlive all SurfaceRefs it issued.
// Typical usage: call prepare() once before a render batch, then acquire() / let refs expire.
class SurfacePoolFast {
public:
    SurfacePoolFast() = default;
    ~SurfacePoolFast();

    // Pre-allocates `count` surfaces of given dimensions into the free list.
    void prepare(uint32_t width, uint32_t height, std::size_t count);

    // Pops from the free list (or allocates a new block as fallback).
    SurfaceRef acquire(uint32_t width, uint32_t height);

    // Destroys all blocks. All SurfaceRefs must already be dead.
    void clear();

    // Called by SurfaceRef destructor when the last reference drops.
    void release_block(SurfaceBlock* block);

private:
    struct BucketKey {
        uint32_t width, height;
        bool operator==(const BucketKey& o) const noexcept {
            return width == o.width && height == o.height;
        }
    };
    struct BucketKeyHash {
        std::size_t operator()(const BucketKey& k) const noexcept {
            return (std::size_t(k.width) << 32) | k.height;
        }
    };
    struct Bucket {
        std::mutex mtx;
        SurfaceBlock* free_list{nullptr};
    };

    Bucket& get_or_create_bucket(uint32_t width, uint32_t height);

    std::mutex m_map_mtx;
    std::unordered_map<BucketKey, std::unique_ptr<Bucket>, BucketKeyHash> m_buckets;

    std::mutex m_all_mtx;
    std::vector<SurfaceBlock*> m_all_blocks;
};

} // namespace tachyon
