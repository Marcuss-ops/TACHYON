# Tracker Module

Computer vision tracking system for motion tracking, optical flow, and feature detection.

## Directory Structure

```
src/tracker/
├── optical_flow.cpp      # Dense optical flow computation
├── feature_tracker.cpp   # Feature detection and tracking (Lucas-Kanade)
├── camera_solver.cpp     # Camera motion solving
├── track.cpp             # Track data structures
└── track_binding.cpp     # Track-to-object bindings
```

## Key Components

### Optical Flow (`optical_flow.cpp`)
- Dense optical flow using pyramidal Lucas-Kanade
- `OpticalFlowResult`: Flow vector containers with confidence
- `OpticalFlowConsumer`: Flow consumption with confidence thresholds
- Pyramid-based flow computation for performance

### Feature Tracking (`feature_tracker.cpp`)
- Harris corner detection
- Pyramidal Lucas-Kanade feature tracking
- Feature point management and lifecycle
- `GrayImage`: Grayscale image representation

### Camera Solving (`camera_solver.cpp`)
- Camera motion estimation from tracks
- 2D-to-3D projection solving
- Keyframe-based camera path generation

## Algorithms

- **Harris Corner Detector**: For feature point detection
- **Lucas-Kanade**: Sparse and dense optical flow
- **Pyramidal Processing**: Multi-scale feature tracking

## Dependencies
- Standard C++ library only (no external CV library required)
