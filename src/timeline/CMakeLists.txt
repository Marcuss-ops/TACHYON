# Tachyon Timeline Module

set(TachyonTimelineSources
    frame_blend.cpp
    frame_evaluator.cpp
    time.cpp
    time_remap.cpp
    time_resolver.cpp
)

tachyon_add_module(
    NAME Timeline
    SOURCES ${TachyonTimelineSources}
    PUBLIC_DEPS TachyonCore
)
