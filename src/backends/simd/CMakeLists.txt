# Tachyon SIMD Backend Module

set(TachyonBackendSimdSources)

if(TACHYON_ENABLE_HIGHWAY)
    list(APPEND TachyonBackendSimdSources
        transition_simd_kernels_highway.cpp
        conversion_highway.cpp
    )
endif()


list(APPEND TachyonBackendSimdSources
    register_simd_backend.cpp
    simd_transition_renderers.cpp
)

if(TachyonBackendSimdSources)
    add_library(TachyonBackendSimd STATIC ${TachyonBackendSimdSources})
    tachyon_configure_common(TachyonBackendSimd)

    target_include_directories(TachyonBackendSimd 
        PUBLIC 
            $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
    )

    target_link_libraries(TachyonBackendSimd
        PUBLIC
            TachyonCore
    )

    if(TACHYON_ENABLE_HIGHWAY)
        if(TARGET hwy)
            target_link_libraries(TachyonBackendSimd PUBLIC hwy)
        elseif(TARGET hwy::hwy)
            target_link_libraries(TachyonBackendSimd PUBLIC hwy::hwy)
        endif()
        target_compile_definitions(TachyonBackendSimd PUBLIC TACHYON_ENABLE_HIGHWAY)
    endif()


    set_target_properties(TachyonBackendSimd PROPERTIES POSITION_INDEPENDENT_CODE ON)
endif()
