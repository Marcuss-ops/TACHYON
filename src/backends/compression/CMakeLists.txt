set(TACHYON_BACKEND_COMPRESSION_SOURCES
    texture_compressor_none.cpp
)

if(TACHYON_ENABLE_BASIS)
    list(APPEND TACHYON_BACKEND_COMPRESSION_SOURCES texture_compressor_basis.cpp)
endif()

if(TACHYON_ENABLE_HIGHWAY)
    list(APPEND TACHYON_BACKEND_COMPRESSION_SOURCES compression_simd_kernels_highway.cpp)
endif()

add_library(TachyonBackendsCompression STATIC ${TACHYON_BACKEND_COMPRESSION_SOURCES})

target_include_directories(TachyonBackendsCompression
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../../../include>
        ${CMAKE_BINARY_DIR}/include
)

target_link_libraries(TachyonBackendsCompression 
    PUBLIC 
        TachyonCore 
        TachyonPlatform
)

if(TACHYON_ENABLE_HIGHWAY)
    set(HWY_TARGET hwy)
    if(TARGET hwy::hwy)
        set(HWY_TARGET hwy::hwy)
    endif()
    if(TARGET ${HWY_TARGET})
        target_link_libraries(TachyonBackendsCompression PUBLIC ${HWY_TARGET})
    endif()
    target_compile_definitions(TachyonBackendsCompression PUBLIC TACHYON_ENABLE_HIGHWAY)
endif()

if(TACHYON_ENABLE_BASIS)
    target_compile_definitions(TachyonBackendsCompression PUBLIC TACHYON_ENABLE_BASIS)
    target_link_libraries(TachyonBackendsCompression PRIVATE basisu_lib)
endif()

set_target_properties(TachyonBackendsCompression PROPERTIES POSITION_INDEPENDENT_CODE ON)
