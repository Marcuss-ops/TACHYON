# Audio Module

Audio processing subsystem for decoding, mixing, analysis, and encoding.

## Directory Structure

```
src/audio/
├── audio_decoder.cpp      # Audio file decoding (FFmpeg)
├── audio_encoder.cpp      # Audio encoding
├── audio_mixer.cpp        # Multi-track audio mixing
├── audio_graph.cpp        # Audio processing graph
├── audio_processor.cpp    # Audio effects processing
├── audio_analyzer.cpp    # Audio analysis (waveforms, etc.)
└── audio_export.cpp       # Audio export utilities
```

## Key Components

### Decoding & Encoding
- `audio_decoder.cpp`: Decodes various audio formats via FFmpeg
- `audio_encoder.cpp`: Encodes audio to output formats

### Processing
- `audio_mixer.cpp`: Mixes multiple audio tracks with volume/pan
- `audio_processor.cpp`: Effects (gain, EQ, filters)
- `audio_analyzer.cpp`: Waveform and spectral analysis

### Export
- `audio_export.cpp`: Export mixed audio to files

## Dependencies
- FFmpeg (libavcodec, libavformat, libavutil, libswresample)

## Note
Previously, audio code was incorrectly placed in `renderer2d/audio/`. It has been moved here for proper separation of concerns.
