# TachyonColor source files
set(TachyonColorSources
    ${CMAKE_SOURCE_DIR}/src/color/color_space.cpp
    ${CMAKE_SOURCE_DIR}/src/color/transfer_functions.cpp
    ${CMAKE_SOURCE_DIR}/src/color/aces.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/color/blending.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/color/blend_kernel.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/color/blend_kernel_avx2.cpp
    ${CMAKE_SOURCE_DIR}/src/color/premultiplied_alpha.cpp
    ${CMAKE_SOURCE_DIR}/src/color/tone_mapping.cpp
)
