# FrameRange Contract

Defines the semantics of frame ranges in Tachyon rendering.

## Definition

```cpp
struct FrameRange {
    std::int64_t start{0};
    std::int64_t end{0};
};
```

Located in: `include/tachyon/runtime/execution/jobs/render_job.h`

## Contract

| Field | Inclusion | Description |
|-------|-----------|-------------|
| `start` | **Inclusive** | First frame to render (>= 0) |
| `end` | **Exclusive** | Frame after last frame to render |

### Frame Count

```
frames = end - start
```

### Duration Conversion

Given a composition with `duration` seconds and `frame_rate` fps:

```
total_frames = ceil(duration * frame_rate)
FrameRange = {0, total_frames}
```

## Examples

| Duration | FPS | Start | End | Frames |
|----------|-----|-------|-----|--------|
| 10s | 30 | 0 | 300 | 300 |
| 10s | 60 | 0 | 600 | 600 |
| 10s | 24 | 0 | 240 | 240 |
| 10.5s | 30 | 0 | 315 | 315 |
| 10.5s | 60 | 0 | 630 | 630 |

## Constraints

- `start >= 0`
- `end >= start` (if `end < start`, the range is invalid and should error)
- Empty range: `start == end` means zero frames to render

## Usage in Code

```cpp
// Creating a range for a 10 second composition at 30fps
double duration = 10.0;
double fps = 30.0;
std::int64_t total_frames = static_cast<std::int64_t>(duration * fps);
FrameRange range{0, total_frames};  // {0, 300}

// Iterating over frames
for (std::int64_t f = range.start; f < range.end; ++f) {
    // render frame f
}
```

## Validation

- If `start > end`: error, invalid frame range
- If `start < 0`: error, negative frame index
- If `end < 0`: error, negative frame index
