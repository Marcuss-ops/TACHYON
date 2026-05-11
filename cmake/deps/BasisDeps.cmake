include_guard(GLOBAL)

if(TACHYON_ENABLE_BASIS)
    if(TACHYON_FETCH_DEPS)
        FetchContent_Declare(
            basis_universal
            GIT_REPOSITORY https://github.com/BinomialLLC/basis_universal.git
            GIT_TAG        ${TACHYON_BASIS_GIT_TAG}
        )
        FetchContent_MakeAvailable(basis_universal)
        
        if(NOT TARGET basisu_lib)
            set(BASISU_SRC_DIR ${basis_universal_SOURCE_DIR})
            add_library(basisu_lib STATIC
                ${BASISU_SRC_DIR}/encoder/basisu_backend.cpp
                ${BASISU_SRC_DIR}/encoder/basisu_basis_file.cpp
                ${BASISU_SRC_DIR}/encoder/basisu_comp.cpp
                ${BASISU_SRC_DIR}/encoder/basisu_enc.cpp
                ${BASISU_SRC_DIR}/encoder/basisu_etc.cpp
                ${BASISU_SRC_DIR}/encoder/basisu_frontend.cpp
                ${BASISU_SRC_DIR}/encoder/basisu_global_selector_palette_helpers.cpp
                ${BASISU_SRC_DIR}/encoder/basisu_gpu_texture.cpp
                ${BASISU_SRC_DIR}/encoder/basisu_pvrtc1_4.cpp
                ${BASISU_SRC_DIR}/encoder/basisu_resampler.cpp
                ${BASISU_SRC_DIR}/encoder/basisu_resample_filters.cpp
                ${BASISU_SRC_DIR}/encoder/basisu_ssim.cpp
                ${BASISU_SRC_DIR}/encoder/basisu_astc_decomp.cpp
                ${BASISU_SRC_DIR}/encoder/basisu_uastc_enc.cpp
                ${BASISU_SRC_DIR}/encoder/basisu_bc7enc.cpp
                ${BASISU_SRC_DIR}/encoder/lodepng.cpp
                ${BASISU_SRC_DIR}/encoder/apg_bmp.c
                ${BASISU_SRC_DIR}/encoder/jpgd.cpp
                ${BASISU_SRC_DIR}/encoder/basisu_kernels_sse.cpp
                ${BASISU_SRC_DIR}/transcoder/basisu_transcoder.cpp
                ${BASISU_SRC_DIR}/zstd/zstd.c
            )
            
            target_include_directories(basisu_lib PUBLIC ${BASISU_SRC_DIR})
            # Default to no SSE to avoid build issues, can be optimized later
            target_compile_definitions(basisu_lib PUBLIC BASISU_SUPPORT_SSE=0)
            
            if(UNIX)
                target_link_libraries(basisu_lib PRIVATE m pthread)
            endif()
            
            set_target_properties(basisu_lib PROPERTIES POSITION_INDEPENDENT_CODE ON)
        endif()
    else()
        find_package(basis_universal REQUIRED)
    endif()
endif()
