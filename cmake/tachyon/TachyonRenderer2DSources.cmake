include(${CMAKE_SOURCE_DIR}/cmake/tachyon/TachyonRenderer2DCoreSources.cmake)
include(${CMAKE_SOURCE_DIR}/cmake/tachyon/TachyonRenderer2DEffectsSources.cmake)
include(${CMAKE_SOURCE_DIR}/cmake/tachyon/TachyonRenderer2DEvaluatedCompositionSources.cmake)
include(${CMAKE_SOURCE_DIR}/cmake/tachyon/TachyonRenderer2DRasterSources.cmake)
include(${CMAKE_SOURCE_DIR}/cmake/tachyon/TachyonRenderer2DExtrasSources.cmake)

set(TachyonRenderer2DSources
    ${TachyonRenderer2DCoreSources}
    ${TachyonRenderer2DEffectsSources}
    ${TachyonRenderer2DEvaluatedCompositionSources}
    ${TachyonRenderer2DRasterSources}
    ${CMAKE_CURRENT_SOURCE_DIR}/core/transition_registry.cpp
)

if(TACHYON_ENABLE_VULKAN)
    list(APPEND TachyonRenderer2DSources
        ${CMAKE_CURRENT_SOURCE_DIR}/renderer2d/backend/vulkan_backend.cpp
    )
endif()
