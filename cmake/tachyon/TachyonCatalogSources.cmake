# TachyonCatalog sources
set(TachyonCatalogSources
    ${CMAKE_CURRENT_SOURCE_DIR}/core/catalog/catalog.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/catalog/transition_catalog.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/catalog/background_catalog.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/catalog/background_registry.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/catalog/backgrounds/solid_background_descriptor.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/catalog/backgrounds/gradient_background_descriptor.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/catalog/backgrounds/radial_gradient_background_descriptor.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/catalog/backgrounds/image_background_descriptor.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/catalog/backgrounds/video_background_descriptor.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/catalog/backgrounds/register_all.cpp
)
