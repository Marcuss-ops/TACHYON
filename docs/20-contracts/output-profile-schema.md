# Output Profile Schema

## Purpose

This document defines the structured output profile contract that render jobs should use.

It exists to prevent output behavior from collapsing into raw backend flags or one-off preset strings with unclear semantics.

## Relationship to other documents

- `render-job.md` explains what a render job is
- `encoder-output.md` explains delivery and encode behavior
- this document defines the schema shape that a render job should use for output intent

## Core rule

A render job should reference either:

- a named built-in output profile
- a fully explicit custom output profile

The engine should normalize both into one internal output contract before encoding starts.

## Schema shape

Recommended first output profile structure:

```yaml
output:
  destination:
    path: "..."
    overwrite: false
  profile:
    name: "delivery_h264_mp4"
    class: "delivery"
    container: "mp4"
    video:
      codec: "h264"
      pixel_format: "yuv420p"
      rate_control_mode: "crf"
      crf: 18
    audio:
      mode: "encode"
      codec: "aac"
      sample_rate: 48000
      channels: 2
    buffering:
      strategy: "pipe"
      max_frames_in_queue: 8
    color:
      transfer: "bt709"
      range: "tv"
```

## Required fields

### Destination
- output path
- overwrite policy

### Profile identity
- profile name or explicit anonymous custom profile
- profile class such as delivery, master, or intermediate

### Container
- mp4, mov, mkv initially

### Video section
- codec
- pixel format
- rate control or quality mode
- codec-specific fields relevant to the selected mode

### Audio section
- none, passthrough, or encode mode
- codec if encode mode is used
- sample rate and channel intent where relevant

### Buffering section
- strategy such as pipe or intermediate_file
- bounded queue size
- temporary storage hints only if required

### Color section
- transfer characteristics expected at output
- range policy if relevant
- later space or matrix metadata if needed

## Profile classes

### Delivery
Used for mainstream final outputs.
Examples:
- H.264 MP4
- H.265 MKV later

### Master
Used for higher-quality archive or handoff outputs.
Examples:
- ProRes MOV

### Intermediate
Used when a workflow intentionally wants a larger but safer or less lossy file.
This class should not become the accidental default.

## Built-in profiles

Recommended first built-ins:

### `delivery_h264_mp4`
- class: delivery
- container: mp4
- codec: h264
- pixel format: yuv420p
- buffering: pipe
- audio: AAC encode by default if audio exists

### `master_prores_mov`
- class: master
- container: mov
- codec: prores
- pixel format: yuv422p10le or equivalent supported target
- buffering: pipe or controlled intermediate depending on backend stability
- audio: PCM or equivalent high-quality encode mode later if supported

### `robust_h264_mkv`
- class: delivery
- container: mkv
- codec: h264
- intended for long-running or interruption-tolerant workflows

## Validation rules

The engine should reject invalid combinations early.

Examples:
- unsupported codec for container
- alpha-preserving request on a profile that discards alpha
- passthrough audio request when no compatible source stream exists
- unbounded queue size in a low-memory environment
- missing rate-control fields for the chosen mode

## Rate control guidance

The first schema should support a narrow set of explicit modes.

Recommended first modes:
- crf
- constant_bitrate later if needed
- quality preset labels only if they normalize into explicit parameters

## Audio modes

The audio section should allow:
- `none`
- `passthrough`
- `encode`

The meaning of each mode must remain explicit.
`passthrough` is a transport choice, not a render semantic shortcut.

## Alpha policy

The output profile must be able to declare whether alpha is:
- discarded
- preserved
- unsupported for the selected target

The engine must not silently drop alpha without the profile making that outcome obvious.

## Buffering policy

The schema must reserve room for output buffering intent.

Recommended first fields:
- strategy
- max frames in queue
- temp directory override later if needed

This keeps low-end performance behavior explicit and reproducible.

## Compatibility rule

A normalized output profile is part of output identity.
If two jobs normalize to different output profiles, they are not the same deliverable even if they use the same scene and seed.

## Guiding principle

Render jobs should express output intent in a small explicit schema.
That keeps FFmpeg integration replaceable, validation strong, and output behavior deterministic.
