# Tachyon Catalog Module (Unified Content + Library)

set(TachyonCatalogSources
    presets/preset_catalog.cpp
    library/background_registry.cpp
    library/backgrounds/solid_background_descriptor.cpp
    library/backgrounds/gradient_background_descriptor.cpp
    library/backgrounds/radial_gradient_background_descriptor.cpp
    library/backgrounds/image_background_descriptor.cpp
    library/backgrounds/video_background_descriptor.cpp
    library/backgrounds/register_all.cpp
    library/backgrounds/background_catalog.cpp
    ../core/library/library.cpp
)

tachyon_add_module(
    NAME Catalog
    SOURCES ${TachyonCatalogSources}
    PUBLIC_DEPS TachyonCore TachyonPresets
)
