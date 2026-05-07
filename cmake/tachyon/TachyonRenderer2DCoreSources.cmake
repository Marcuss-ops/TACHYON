# TachyonRenderer2D core sources
include(${CMAKE_SOURCE_DIR}/cmake/tachyon/TachyonRenderer2DBackendSources.cmake)
include(${CMAKE_SOURCE_DIR}/cmake/tachyon/TachyonRenderer2DColorSources.cmake)
include(${CMAKE_SOURCE_DIR}/cmake/tachyon/TachyonRenderer2DSurfaceSources.cmake)
include(${CMAKE_SOURCE_DIR}/cmake/tachyon/TachyonRenderer2DPathSources.cmake)
include(${CMAKE_SOURCE_DIR}/cmake/tachyon/TachyonRenderer2DResourceSources.cmake)

set(TachyonRenderer2DCoreSources
    ${TachyonRenderer2DBackendSources}
    ${TachyonRenderer2DSurfaceSources}
    ${TachyonRenderer2DPathSources}
    ${TachyonRenderer2DResourceSources}
    ${CMAKE_CURRENT_SOURCE_DIR}/renderer2d/audio/audio_sampling.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/renderer2d/deform/mesh_deform.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/renderer2d/math/math_utils.cpp
)
