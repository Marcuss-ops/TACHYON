#include "tachyon/runtime/resource/surface_pool_fast.h"

namespace tachyon {

// ---------------------------------------------------------------------------
// SurfaceRef
// ---------------------------------------------------------------------------

void SurfaceRef::release() {
    if (!block_) return;
    if (block_->refcount.fetch_sub(1, std::memory_order_acq_rel) == 1) {
        block_->owner->release_block(block_);
    }
    block_ = nullptr;
}

SurfaceRef& SurfaceRef::operator=(const SurfaceRef& o) {
    if (this == &o) return *this;
    release();
    block_ = o.block_;
    if (block_) block_->refcount.fetch_add(1, std::memory_order_relaxed);
    return *this;
}

SurfaceRef& SurfaceRef::operator=(SurfaceRef&& o) noexcept {
    if (this == &o) return *this;
    release();
    block_ = o.block_;
    o.block_ = nullptr;
    return *this;
}

// ---------------------------------------------------------------------------
// SurfacePoolFast
// ---------------------------------------------------------------------------

SurfacePoolFast::~SurfacePoolFast() {
    clear();
}

SurfacePoolFast::Bucket& SurfacePoolFast::get_or_create_bucket(uint32_t width, uint32_t height) {
    BucketKey key{width, height};
    std::lock_guard lock(m_map_mtx);
    auto it = m_buckets.find(key);
    if (it != m_buckets.end()) return *it->second;
    auto [ins, ok] = m_buckets.emplace(key, std::make_unique<Bucket>());
    return *ins->second;
}

void SurfacePoolFast::prepare(uint32_t width, uint32_t height, std::size_t count) {
    Bucket& bucket = get_or_create_bucket(width, height);

    std::vector<SurfaceBlock*> new_blocks;
    new_blocks.reserve(count);
    for (std::size_t i = 0; i < count; ++i) {
        auto* b = new SurfaceBlock(width, height);
        b->owner = this;
        new_blocks.push_back(b);
    }

    {
        std::lock_guard lock(m_all_mtx);
        m_all_blocks.insert(m_all_blocks.end(), new_blocks.begin(), new_blocks.end());
    }
    {
        std::lock_guard lock(bucket.mtx);
        for (auto* b : new_blocks) {
            b->next = bucket.free_list;
            bucket.free_list = b;
        }
    }
}

SurfaceRef SurfacePoolFast::acquire(uint32_t width, uint32_t height) {
    Bucket& bucket = get_or_create_bucket(width, height);

    SurfaceBlock* b = nullptr;
    {
        std::lock_guard lock(bucket.mtx);
        b = bucket.free_list;
        if (b) {
            bucket.free_list = b->next;
            b->next = nullptr;
        }
    }

    if (!b) {
        b = new SurfaceBlock(width, height);
        b->owner = this;
        std::lock_guard lock(m_all_mtx);
        m_all_blocks.push_back(b);
    }

    b->refcount.store(1, std::memory_order_relaxed);
    return SurfaceRef(b);
}

void SurfacePoolFast::release_block(SurfaceBlock* block) {
    if (!block) return;
    Bucket& bucket = get_or_create_bucket(
        block->surface.width(), block->surface.height());
    std::lock_guard lock(bucket.mtx);
    block->next = bucket.free_list;
    bucket.free_list = block;
}

void SurfacePoolFast::clear() {
    {
        std::lock_guard map_lock(m_map_mtx);
        for (auto& [key, bucket] : m_buckets) {
            std::lock_guard bkt_lock(bucket->mtx);
            bucket->free_list = nullptr;
        }
        m_buckets.clear();
    }
    std::lock_guard lock(m_all_mtx);
    for (auto* b : m_all_blocks) delete b;
    m_all_blocks.clear();
}

} // namespace tachyon
