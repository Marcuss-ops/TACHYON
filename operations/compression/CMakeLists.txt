set(TACHYON_COMPRESSION_OPS_SOURCES
    src/texture_compressor_none.cpp
    src/texture_compressor_basis.cpp
    src/compression_simd_kernels_highway.cpp
)

add_library(TachyonCompressionOps STATIC ${TACHYON_COMPRESSION_OPS_SOURCES})
target_include_directories(TachyonCompressionOps
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_BINARY_DIR}/include
)
target_link_libraries(TachyonCompressionOps PUBLIC TachyonCore TachyonPlatform pugixml)

if(TACHYON_ENABLE_HIGHWAY)
    set(HWY_TARGET hwy)
    if(TARGET hwy::hwy)
        set(HWY_TARGET hwy::hwy)
    endif()
    if(TARGET ${HWY_TARGET})
        target_link_libraries(TachyonCompressionOps PUBLIC ${HWY_TARGET})
    endif()
    target_compile_definitions(TachyonCompressionOps PUBLIC TACHYON_ENABLE_HIGHWAY)
endif()

if(TACHYON_ENABLE_BASIS)
    target_compile_definitions(TachyonCompressionOps PUBLIC TACHYON_ENABLE_BASIS)
    target_link_libraries(TachyonCompressionOps PRIVATE basisu_lib)
endif()

set_target_properties(TachyonCompressionOps PROPERTIES POSITION_INDEPENDENT_CODE ON)
