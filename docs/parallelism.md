# Multi-Threaded Parallel Rendering

TACHYON is designed from the ground up to achieve maximum multi-core CPU saturation. Since it bypasses browser DOM architectures, the engine utilizes high-fidelity multi-threaded pipelines to scale performance linearly with available processor cores.

---

## 1. Parallelization Architecture

Tachyon processes parallel work at four distinct layers, starting from batch job execution down to tile-based pixel rasterization:

```
┌────────────────────────────────────────────────────────┐
│  Level 1: Batch Parallelism (Multi-Video Rendering)    │
└───────────────────────────┬────────────────────────────┘
                            ▼
┌────────────────────────────────────────────────────────┐
│  Level 2: Frame Parallelism (Lookahead Queue)          │
└───────────────────────────┬────────────────────────────┘
                            ▼
┌────────────────────────────────────────────────────────┐
│  Level 3: Tile Parallelism (Work-Stealing Tile Grid)   │
└───────────────────────────┬────────────────────────────┘
                            ▼
┌────────────────────────────────────────────────────────┐
│  Level 4: Layer Parallelism (Independent Rasterization)│
└────────────────────────────────────────────────────────┘
```

---

## 2. Technical Implementation Details

### Level 1: Batch Parallelism
For automated video workflows (e.g., generating multiple variations of ads, localized promos, or personalized content), multiple render jobs are executed concurrently using the `BatchRunner`:
- **Scheduler**: Thread pool orchestrated via `RenderBatchSpec::worker_count`.
- **Isolation**: Each worker threads its own execution context. To guarantee thread safety, `FrameCache` and `MediaManager` instances are strictly local to each running job (zero shared mutable state).

```cpp
// Orchestrated via run_render_batch()
RenderBatchResult result;
BS::thread_pool pool(worker_count);
std::vector<std::future<RenderBatchJobResult>> futures;

for (const auto& job : spec.jobs) {
    futures.push_back(pool.submit([job]() {
        return run_single_job(job);
    }));
}
```

### Level 2: Frame Parallelism
Within a single video composition, multiple sequential frames are evaluated and rendered concurrently:
- **Lookahead Queue**: A lock-free window queue schedules upcoming frames to stay ahead of the video encoder.
- **Ordered Encoding**: While frame rendering is asynchronous, the final video encoder receives completed frames in strict sequential order via an ordering queue to prevent out-of-order writes.
- **Thread Safety**: All access to external visual assets is locked via a thread-safe `FrameCache` (mutex-protected) and a thread-isolated decoder pool inside the `MediaManager`.

### Level 3: Tile Parallelism
For high-resolution outputs (such as 4K video streams), the screen framebuffer is subdivided into rectangular chunks called **Tiles**:
- **Grid Layout**: Handled dynamically using `TileGrid` (default size: 256x256 pixels).
- **Work-Stealing Scheduling**: Individual tiles are pushed onto a work queue where worker threads grab and render them independently.
- **Clipping Boundaries**: The compositor limits rasterization strictly to the active tile viewport using hardware/SIMD bounding-box clipping, avoiding redundant pixel arithmetic.

### Level 4: Layer Parallelism
Individual compositing layers inside a single composition frame can be parallelized when there are no temporal or alpha blending dependencies:
- **Dependency Graph**: The `DependencyGraph` dynamically inspects the layer hierarchy to group layers that can be rasterized independently (e.g. layers without active matte, mask, or track matte associations).
- **Independent Buffers**: Independent layers are rasterized into separate isolated buffers concurrently before the main compositor performs final alpha blending onto the main framebuffer.

---

## 3. Memory Budgeting & Pool Policies

To prevent out-of-memory crashes when rendering highly complex timelines with massive image assets, Tachyon enforces strict limits via the `BatchMemoryPolicy`:

```cpp
struct BatchMemoryPolicy {
    size_t max_total_ram_bytes{4ULL * 1024 * 1024 * 1024};       // 4GB global threshold
    size_t per_job_frame_cache_bytes{512ULL * 1024 * 1024};      // 512MB per job
    size_t per_job_asset_cache_bytes{1ULL * 1024 * 1024 * 1024}; // 1GB per job
};
```

Before dispatching subsequent jobs or lookahead frame threads, the memory coordinator verifies current allocation usage. If resource limits are exceeded, lookahead threads are choked back and dynamic page flushing is triggered inside the `SurfacePool`.
