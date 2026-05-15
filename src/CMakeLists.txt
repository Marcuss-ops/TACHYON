include(${CMAKE_SOURCE_DIR}/cmake/tachyon/TachyonPresetsSources.cmake)
include(${CMAKE_SOURCE_DIR}/cmake/tachyon/ModuleContent.cmake)
include(${CMAKE_SOURCE_DIR}/cmake/tachyon/TachyonCoreSources.cmake)
include(${CMAKE_SOURCE_DIR}/cmake/tachyon/TachyonBindingsCSources.cmake)
include(${CMAKE_SOURCE_DIR}/cmake/tachyon/TachyonDiagnosticsSources.cmake)
include(${CMAKE_SOURCE_DIR}/cmake/tachyon/TachyonPlatformSources.cmake)
include(${CMAKE_SOURCE_DIR}/cmake/tachyon/TachyonSceneEvalSources.cmake)
include(${CMAKE_SOURCE_DIR}/cmake/tachyon/TachyonTimelineSources.cmake)

# include(${CMAKE_SOURCE_DIR}/cmake/tachyon/TachyonMediaSources.cmake)
# include(${CMAKE_SOURCE_DIR}/cmake/tachyon/TachyonAudioSources.cmake)
include(${CMAKE_SOURCE_DIR}/cmake/tachyon/TachyonOutputSources.cmake)
include(${CMAKE_SOURCE_DIR}/cmake/tachyon/TachyonTextSources.cmake)
include(${CMAKE_SOURCE_DIR}/cmake/tachyon/TachyonLibrarySources.cmake)
include(${CMAKE_SOURCE_DIR}/cmake/tachyon/TachyonColorSources.cmake)

include(${CMAKE_SOURCE_DIR}/cmake/tachyon/SourceRegistrationCheck.cmake)

# --- Functions ---

function(tachyon_assert_no_duplicate_sources var_name)
    set(_list "${${var_name}}")
    set(_seen)
    set(_duplicates)
    foreach(_source IN LISTS _list)
        list(FIND _seen "${_source}" _seen_index)
        if(_seen_index EQUAL -1)
            list(APPEND _seen "${_source}")
        else()
            list(FIND _duplicates "${_source}" _duplicate_index)
            if(_duplicate_index EQUAL -1)
                list(APPEND _duplicates "${_source}")
            endif()
        endif()
    endforeach()

    list(LENGTH _duplicates _duplicate_count)
    if(_duplicate_count GREATER 0)
        string(REPLACE ";" "\n  " _duplicate_lines "${_duplicates}")
        message(FATAL_ERROR "Duplicate sources found in ${var_name}:\n  ${_duplicate_lines}")
    endif()
endfunction()

include_directories("${CMAKE_SOURCE_DIR}/src")
include_directories("${CMAKE_SOURCE_DIR}/include")

function(tachyon_configure_common target)
    target_include_directories(${target}
        PRIVATE
            "${CMAKE_SOURCE_DIR}/src"
        PUBLIC
            "${CMAKE_SOURCE_DIR}/include"
            "${CMAKE_BINARY_DIR}/include"
            "${CMAKE_SOURCE_DIR}/third_party"
            "${CMAKE_SOURCE_DIR}/third_party/stb/include"
    )
    
    if(absl_SOURCE_DIR)
        target_include_directories(${target} PUBLIC ${absl_SOURCE_DIR})
    endif()

    if(MSVC)
        target_compile_options(${target} PRIVATE /W4 /permissive- /utf-8 /FS)
        if(TACHYON_TREAT_WARNINGS_AS_ERRORS)
            target_compile_options(${target} PRIVATE /WX)
        endif()
    else()
        target_compile_options(${target} PRIVATE -Wall -Wextra -Wpedantic)
        if(TACHYON_TREAT_WARNINGS_AS_ERRORS)
            target_compile_options(${target} PRIVATE -Werror)
        endif()
    endif()

    set_target_properties(${target} PROPERTIES POSITION_INDEPENDENT_CODE ON)
    target_compile_definitions(${target} PRIVATE TACHYON_BUILD_DLL)
endfunction()

function(tachyon_link_absl target)
    target_link_libraries(${target} PUBLIC 
        absl_flat_hash_map
        absl_strings
        absl_str_format
    )
    if(absl_SOURCE_DIR)
        target_include_directories(${target} PUBLIC ${absl_SOURCE_DIR})
    endif()
endfunction()

function(tachyon_link_text_deps target)
    if(TACHYON_ENABLE_TEXT)
        target_link_libraries(${target} PUBLIC ${TACHYON_FREETYPE_TARGET} ${TACHYON_HARFBUZZ_TARGET})
        target_compile_definitions(${target} PUBLIC TACHYON_ENABLE_TEXT)
        target_link_libraries(${target} PRIVATE TachyonSkiaBackend)
    endif()
endfunction()

function(tachyon_link_media_deps target)
    if(TACHYON_ENABLE_MEDIA)
        find_package(PkgConfig QUIET)
        if(PKG_CONFIG_FOUND)
            pkg_check_modules(FFMPEG QUIET libavcodec libavformat libavutil libswscale libswresample)
            if(FFMPEG_FOUND)
                target_link_libraries(${target} PUBLIC ${FFMPEG_LIBRARIES})
                target_include_directories(${target} PUBLIC ${FFMPEG_INCLUDE_DIRS})
            endif()
        endif()
        target_compile_definitions(${target} PUBLIC TACHYON_ENABLE_MEDIA)
    endif()
endfunction()

function(tachyon_link_omp target)
    if(TACHYON_HAS_OPENMP AND TARGET OpenMP::OpenMP_CXX)
        target_link_libraries(${target} PUBLIC OpenMP::OpenMP_CXX)
    endif()
endfunction()

function(tachyon_link_output_deps target)
    if(TACHYON_ENABLE_OUTPUT)
        target_compile_definitions(${target} PUBLIC TACHYON_ENABLE_OUTPUT)

        if(TACHYON_ENABLE_OIDN AND WIN32)
            target_include_directories(${target} PUBLIC $<BUILD_INTERFACE:${oidn_SOURCE_DIR}/include>)
            target_link_libraries(${target} PUBLIC OpenImageDenoise)
        endif()
    endif()
endfunction()

# --- Base Targets ---

add_library(TachyonCore STATIC ${TachyonCoreSources})
tachyon_configure_common(TachyonCore)
tachyon_link_omp(TachyonCore)
tachyon_link_absl(TachyonCore)
add_subdirectory(${CMAKE_SOURCE_DIR}/operations ${CMAKE_BINARY_DIR}/operations)

add_library(TachyonContent STATIC ${TACHYON_CONTENT_SOURCES})
tachyon_configure_common(TachyonContent)
target_link_libraries(TachyonContent PUBLIC TachyonCore)

if(TACHYON_ENABLE_TRACY)
    target_link_libraries(TachyonCore PUBLIC TracyClient)
    target_compile_definitions(TachyonCore PUBLIC TACHYON_ENABLE_TRACY)
endif()

if(TACHYON_ENABLE_SIMDJSON)
    target_compile_definitions(TachyonCore PUBLIC TACHYON_ENABLE_SIMDJSON)

    if(TARGET simdjson::simdjson)
        target_link_libraries(TachyonCore PUBLIC simdjson::simdjson)
    elseif(TARGET simdjson)
        target_link_libraries(TachyonCore PUBLIC simdjson)
    endif()
endif()

if(TACHYON_ENABLE_HIGHWAY)
    if(TARGET hwy)
        target_link_libraries(TachyonCore PUBLIC hwy)
    elseif(TARGET hwy::hwy)
        target_link_libraries(TachyonCore PUBLIC hwy::hwy)
    else()
        message(FATAL_ERROR "[Tachyon] Highway enabled but no Highway CMake target was found")
    endif()

    target_compile_definitions(TachyonCore PUBLIC TACHYON_ENABLE_HIGHWAY)
endif()

target_include_directories(TachyonCore
    PUBLIC
        $<BUILD_INTERFACE:${clipper2_SOURCE_DIR}/CPP/Clipper2Lib/include>
)
target_sources(TachyonCore PRIVATE
    ${clipper2_SOURCE_DIR}/CPP/Clipper2Lib/src/clipper.engine.cpp
    ${clipper2_SOURCE_DIR}/CPP/Clipper2Lib/src/clipper.offset.cpp
    ${clipper2_SOURCE_DIR}/CPP/Clipper2Lib/src/clipper.rectclip.cpp
)

if(TACHYON_ENABLE_MANUAL_AVX2)
    target_sources(TachyonCore PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/core/transition/transition_simd_kernels_avx2.cpp
    )
    if(MSVC)
        set_source_files_properties(core/transition/transition_simd_kernels_avx2.cpp PROPERTIES COMPILE_FLAGS "/arch:AVX2")
    else()
        set_source_files_properties(core/transition/transition_simd_kernels_avx2.cpp PROPERTIES COMPILE_FLAGS "-mavx2")
    endif()
    target_compile_definitions(TachyonCore PRIVATE "TACHYON_AVX2")
endif()

add_library(TachyonColor STATIC ${TachyonColorSources})
tachyon_configure_common(TachyonColor)
target_link_libraries(TachyonColor PUBLIC TachyonCore)

if(MSVC)
    set_source_files_properties(color/blend_kernel_avx2.cpp PROPERTIES COMPILE_FLAGS "/arch:AVX2")
    target_compile_definitions(TachyonColor PRIVATE "TACHYON_AVX2")
endif()

add_library(TachyonBindingsC STATIC ${TachyonBindingsCSources})
tachyon_configure_common(TachyonBindingsC)
target_link_libraries(TachyonBindingsC PUBLIC TachyonCore)

add_library(TachyonDiagnostics STATIC ${TachyonDiagnosticsSources})
tachyon_configure_common(TachyonDiagnostics)
target_link_libraries(TachyonDiagnostics PUBLIC TachyonCore)

if(TACHYON_ENABLE_TRACE)
    target_compile_definitions(TachyonDiagnostics PUBLIC TACHYON_ENABLE_TRACE)
endif()

if(TACHYON_ENABLE_JSON_TRACE)
    target_compile_definitions(TachyonDiagnostics PUBLIC TACHYON_ENABLE_JSON_TRACE)
endif()

if(TACHYON_ENABLE_PERFETTO)
    target_compile_definitions(TachyonDiagnostics PUBLIC TACHYON_ENABLE_PERFETTO)
    target_link_libraries(TachyonDiagnostics PRIVATE perfetto)
endif()

add_library(TachyonTimeline STATIC ${TachyonTimelineSources})
tachyon_configure_common(TachyonTimeline)
target_link_libraries(TachyonTimeline PUBLIC TachyonCore)

# --- Module Includes (Collected sources and module targets) ---

include(${CMAKE_SOURCE_DIR}/cmake/tachyon/ModuleScene.cmake)
include(${CMAKE_SOURCE_DIR}/cmake/tachyon/ModuleRenderer2D.cmake)

# TachyonMedia and TachyonAudio moved to operations/

if(TACHYON_ENABLE_OUTPUT)
    add_library(TachyonOutput STATIC ${TachyonOutputSources})
    tachyon_configure_common(TachyonOutput)
    target_include_directories(TachyonOutput
        PRIVATE
            ${tinyexr_SOURCE_DIR}
    )
    tachyon_link_media_deps(TachyonOutput)
    tachyon_link_output_deps(TachyonOutput)
    target_link_libraries(TachyonOutput PUBLIC TachyonColor)
endif()

include(${CMAKE_SOURCE_DIR}/cmake/tachyon/ModuleRuntime.cmake)
include(${CMAKE_SOURCE_DIR}/cmake/tachyon/ModuleServer.cmake)

# --- Post-Module Configuration & Asserts ---

tachyon_assert_no_duplicate_sources(TachyonSceneSources)
tachyon_assert_no_duplicate_sources(TachyonSceneEvalSources)
tachyon_assert_no_duplicate_sources(TachyonTimelineSources)
tachyon_assert_no_duplicate_sources(TachyonBindingsCSources)
tachyon_assert_no_duplicate_sources(TachyonDiagnosticsSources)
tachyon_assert_no_duplicate_sources(TachyonPresetsSources)
tachyon_assert_no_duplicate_sources(TachyonRuntimeSources)
tachyon_assert_no_duplicate_sources(TachyonRenderer2DSources)
tachyon_assert_no_duplicate_sources(TachyonRenderer2DExtrasSources)
# tachyon_assert_no_duplicate_sources(TachyonMediaSources)
# tachyon_assert_no_duplicate_sources(TachyonAudioSources)
tachyon_assert_no_duplicate_sources(TachyonOutputSources)
tachyon_assert_no_duplicate_sources(TachyonTextSources)
tachyon_assert_no_duplicate_sources(TachyonLibrarySources)

tachyon_check_source_registration(${CMAKE_CURRENT_SOURCE_DIR}/text TachyonTextSources)
# tachyon_check_source_registration(${CMAKE_CURRENT_SOURCE_DIR}/media TachyonMediaSources)
tachyon_check_source_registration(${CMAKE_CURRENT_SOURCE_DIR}/renderer2d TachyonRenderer2DAllSources)
tachyon_check_source_registration(${CMAKE_CURRENT_SOURCE_DIR}/runtime TachyonRuntimeSources)
tachyon_check_source_registration(${CMAKE_CURRENT_SOURCE_DIR}/core/scene TachyonSceneAllSources)

# Global check for cross-target source duplication
set(GlobalTachyonSources
    ${TachyonCoreSources}
    ${TachyonTimelineSources}
    ${TachyonSceneEvalSources}
    ${TachyonSceneSources}
    ${TachyonRuntimeSources}
    ${TachyonRenderer2DSources}
    ${TachyonDiagnosticsSources}
    ${TachyonPresetsSources}
    ${TachyonPlatformSources}
    ${TachyonColorSources}
)

if(TACHYON_ENABLE_OUTPUT)
    list(APPEND GlobalTachyonSources ${TachyonOutputSources})
endif()
if(TACHYON_ENABLE_TEXT)
    list(APPEND GlobalTachyonSources ${TachyonTextSources})
endif()

tachyon_assert_no_duplicate_sources(GlobalTachyonSources)

if(TACHYON_ENABLE_TEXT)
    add_library(TachyonText STATIC ${TachyonTextSources})
    tachyon_configure_common(TachyonText)
    tachyon_link_text_deps(TachyonText)
    target_link_libraries(TachyonText PUBLIC TachyonCore)
endif()



add_library(TachyonPlatform STATIC ${TachyonPlatformSources})
tachyon_configure_common(TachyonPlatform)

add_library(TachyonLibrary STATIC ${TachyonLibrarySources})
tachyon_configure_common(TachyonLibrary)
target_link_libraries(TachyonLibrary PUBLIC TachyonPresets)

# CLI Module
add_subdirectory(cli)

# Minimal render-focused executable
add_executable(tachyon
    main.cpp
)
tachyon_configure_common(tachyon)

if(TACHYON_ENABLE_PCH AND COMMAND target_precompile_headers)
    target_precompile_headers(tachyon PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/../include/tachyon/pch.h")
endif()

target_link_libraries(tachyon
    PRIVATE
        TachyonCLICore TachyonCLITools
        TachyonLibrary
        TachyonPresets
        TachyonDiagnostics
        TachyonOperations
)
tachyon_link_mimalloc(tachyon)

# Standalone library demo tool
add_executable(tachyon-library-demo
    ${CMAKE_SOURCE_DIR}/tools/library_demo/main.cpp
)
tachyon_configure_common(tachyon-library-demo)
target_link_libraries(tachyon-library-demo
    PRIVATE
        TachyonCLICore TachyonCLITools
        TachyonLibrary
        TachyonPresets
        TachyonDiagnostics
)
tachyon_link_mimalloc(tachyon-library-demo)

# Full tooling/library executable (optional)
if(TACHYON_BUILD_LIBRARY_TOOLS)
    add_executable(tachyon-library
        main.cpp
    )
    tachyon_configure_common(tachyon-library)
    
    if(TACHYON_ENABLE_PCH AND COMMAND target_precompile_headers)
        target_precompile_headers(tachyon-library PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/../include/tachyon/pch.h")
    endif()

    target_link_libraries(tachyon-library
        PRIVATE
        TachyonCLICore TachyonCLITools
            TachyonRuntimeEngine
            TachyonLibrary
            TachyonPresets
            TachyonDiagnostics
    )
    tachyon_link_mimalloc(tachyon-library)
endif()

if(MSVC)
    file(GLOB OIDN_RUNTIME_DLLS CONFIGURE_DEPENDS "${oidn_SOURCE_DIR}/bin/*.dll")
    if(OIDN_RUNTIME_DLLS)
        add_custom_command(TARGET tachyon POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    ${OIDN_RUNTIME_DLLS}
                    $<TARGET_FILE_DIR:tachyon>
            COMMAND_EXPAND_LISTS
        )
    endif()
endif()
