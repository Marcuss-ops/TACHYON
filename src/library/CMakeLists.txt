# Tachyon Library Module

set(TachyonLibrarySources
    ../core/library/library.cpp
    background_registry.cpp
    backgrounds/solid_background_descriptor.cpp
    backgrounds/gradient_background_descriptor.cpp
    backgrounds/radial_gradient_background_descriptor.cpp
    backgrounds/image_background_descriptor.cpp
    backgrounds/video_background_descriptor.cpp
    backgrounds/register_all.cpp
    backgrounds/background_catalog.cpp
)

tachyon_add_module(
    NAME Library
    SOURCES ${TachyonLibrarySources}
    PUBLIC_DEPS TachyonPresets
)
