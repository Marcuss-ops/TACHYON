# Tachyon Color Module

set(TachyonColorSources
    color_space.cpp
    color_math.cpp
    transfer_functions.cpp
    aces.cpp
    blending.cpp
    blend_kernel.cpp
    blend_kernel_avx2.cpp
    premultiplied_alpha.cpp
    tone_mapping.cpp
)

tachyon_add_module(
    NAME Color
    SOURCES ${TachyonColorSources}
    PUBLIC_DEPS TachyonCore
)

if(MSVC)
    set_source_files_properties(blend_kernel_avx2.cpp PROPERTIES COMPILE_FLAGS "/arch:AVX2")
    target_compile_definitions(TachyonColor PRIVATE "TACHYON_AVX2")
endif()
