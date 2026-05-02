# TachyonTracker sources
set(TachyonTrackerSources
    ${CMAKE_CURRENT_SOURCE_DIR}/tracker/track_binding.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/tracker/stabilizer.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/tracker/point_tracker.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/tracker/track.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/tracker/feature_tracker.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/tracker/optical_flow.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/tracker/planar_track.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/tracker/planar_track_binding.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/tracker/camera_solver.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/tracker/rolling_shutter.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/tracker/planar_tracker.cpp
    
    ${CMAKE_CURRENT_SOURCE_DIR}/tracker/core/track_binding.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/tracker/core/track.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/tracker/core/planar_track.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/tracker/core/planar_track_binding.cpp

    ${CMAKE_CURRENT_SOURCE_DIR}/tracker/algorithms/lk_tracker.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/tracker/algorithms/optical_flow_math.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/tracker/algorithms/stabilizer.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/tracker/algorithms/point_tracker.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/tracker/algorithms/feature_tracker.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/tracker/algorithms/corner_detector.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/tracker/algorithms/optical_flow.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/tracker/algorithms/camera_solver_keyframes.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/tracker/algorithms/camera_solver_math.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/tracker/algorithms/motion_estimation.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/tracker/algorithms/optical_flow_translation.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/tracker/algorithms/camera_solver.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/tracker/algorithms/track_utils.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/tracker/algorithms/image_pyramid.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/tracker/algorithms/rolling_shutter.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/tracker/algorithms/planar_tracker.cpp
)
