include(${CMAKE_SOURCE_DIR}/cmake/tachyon/TachyonPresetsSources.cmake)
include(${CMAKE_SOURCE_DIR}/cmake/tachyon/TachyonCoreSources.cmake)
include(${CMAKE_SOURCE_DIR}/cmake/tachyon/TachyonBindingsCSources.cmake)
include(${CMAKE_SOURCE_DIR}/cmake/tachyon/TachyonDiagnosticsSources.cmake)
include(${CMAKE_SOURCE_DIR}/cmake/tachyon/TachyonPlatformSources.cmake)
include(${CMAKE_SOURCE_DIR}/cmake/tachyon/TachyonSceneEvalSources.cmake)
include(${CMAKE_SOURCE_DIR}/cmake/tachyon/TachyonTimelineSources.cmake)

include(${CMAKE_SOURCE_DIR}/cmake/tachyon/TachyonRenderer3DSources.cmake)
include(${CMAKE_SOURCE_DIR}/cmake/tachyon/TachyonMediaSources.cmake)
include(${CMAKE_SOURCE_DIR}/cmake/tachyon/TachyonAudioSources.cmake)
include(${CMAKE_SOURCE_DIR}/cmake/tachyon/TachyonOutputSources.cmake)
include(${CMAKE_SOURCE_DIR}/cmake/tachyon/TachyonTextSources.cmake)
include(${CMAKE_SOURCE_DIR}/cmake/tachyon/TachyonEditorSources.cmake)
include(${CMAKE_SOURCE_DIR}/cmake/tachyon/TachyonCLISources.cmake)
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

function(tachyon_configure_common target)
    target_include_directories(${target}
        PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/..
        PUBLIC
            $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../include>
            $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/include>
            $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../third_party>
            $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../third_party/stb/include>
            $<INSTALL_INTERFACE:include>
    )

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

function(tachyon_link_3d_deps target)
    if(TACHYON_ENABLE_3D)
        target_compile_definitions(${target} PUBLIC TACHYON_ENABLE_3D)
        if(TACHYON_ENABLE_FILAMENT)
            target_compile_definitions(${target} PUBLIC TACHYON_ENABLE_FILAMENT)
        endif()
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

        if(TACHYON_ENABLE_OIDN AND WIN32 AND TACHYON_ENABLE_3D)
            target_include_directories(${target} PUBLIC $<BUILD_INTERFACE:${oidn_SOURCE_DIR}/include>)
            target_link_libraries(${target} PUBLIC OpenImageDenoise)
        endif()
    endif()
endfunction()

# --- Base Targets ---

add_library(TachyonCore STATIC ${TachyonCoreSources})
tachyon_configure_common(TachyonCore)
tachyon_link_omp(TachyonCore)

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

add_library(TachyonTimeline STATIC ${TachyonTimelineSources})
tachyon_configure_common(TachyonTimeline)
target_link_libraries(TachyonTimeline PUBLIC TachyonCore)
if(TACHYON_ENABLE_TRACKER)
    target_link_libraries(TachyonTimeline PUBLIC TachyonTracker)
endif()

# --- Module Includes (Collected sources and module targets) ---

include(${CMAKE_SOURCE_DIR}/cmake/tachyon/ModuleScene.cmake)
include(${CMAKE_SOURCE_DIR}/cmake/tachyon/ModuleRenderer2D.cmake)

add_library(TachyonMedia STATIC ${TachyonMediaSources})
tachyon_configure_common(TachyonMedia)
target_include_directories(TachyonMedia
    PRIVATE
        ${tinygltf_SOURCE_DIR}
        ${pugixml_SOURCE_DIR}/src
        ${TACHYON_EARCUT_INCLUDE_DIR}
)
tachyon_link_media_deps(TachyonMedia)
target_link_libraries(TachyonMedia PUBLIC pugixml TachyonText TachyonRenderer2D)

add_library(TachyonAudio STATIC ${TachyonAudioSources})
tachyon_configure_common(TachyonAudio)
tachyon_link_media_deps(TachyonAudio)

add_library(TachyonOutput STATIC ${TachyonOutputSources})
tachyon_configure_common(TachyonOutput)
target_include_directories(TachyonOutput
    PRIVATE
        ${tinyexr_SOURCE_DIR}
)
tachyon_link_media_deps(TachyonOutput)
tachyon_link_output_deps(TachyonOutput)
target_link_libraries(TachyonOutput PUBLIC TachyonColor)

include(${CMAKE_SOURCE_DIR}/cmake/tachyon/ModuleRuntime.cmake)

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
tachyon_assert_no_duplicate_sources(TachyonRenderer3DSources)
tachyon_assert_no_duplicate_sources(TachyonMediaSources)
tachyon_assert_no_duplicate_sources(TachyonAudioSources)
tachyon_assert_no_duplicate_sources(TachyonOutputSources)
tachyon_assert_no_duplicate_sources(TachyonTextSources)
tachyon_assert_no_duplicate_sources(TachyonEditorSources)
tachyon_assert_no_duplicate_sources(TachyonCLICoreSources)
tachyon_assert_no_duplicate_sources(TachyonCLILibrarySources)
tachyon_assert_no_duplicate_sources(TachyonLibrarySources)

tachyon_check_source_registration(${CMAKE_CURRENT_SOURCE_DIR}/text TachyonTextSources)
tachyon_check_source_registration(${CMAKE_CURRENT_SOURCE_DIR}/media TachyonMediaSources)
tachyon_check_source_registration(${CMAKE_CURRENT_SOURCE_DIR}/renderer2d TachyonRenderer2DAllSources)
tachyon_check_source_registration(${CMAKE_CURRENT_SOURCE_DIR}/runtime TachyonRuntimeSources)
tachyon_check_source_registration(${CMAKE_CURRENT_SOURCE_DIR}/core/scene TachyonSceneAllSources)

if(TACHYON_ENABLE_TRACKER)
    include(${CMAKE_SOURCE_DIR}/cmake/tachyon/TachyonTrackerSources.cmake)
    tachyon_check_source_registration(${CMAKE_CURRENT_SOURCE_DIR}/tracker TachyonTrackerSources)
    tachyon_assert_no_duplicate_sources(TachyonTrackerSources)
endif()

# Global check for cross-target source duplication
set(GlobalTachyonSources
    ${TachyonCoreSources}
    ${TachyonTimelineSources}
    ${TachyonSceneEvalSources}
    ${TachyonSceneSources}
    ${TachyonRuntimeSources}
    ${TachyonRenderer2DSources}
    ${TachyonMediaSources}
    ${TachyonAudioSources}
    ${TachyonOutputSources}
    ${TachyonTextSources}
    ${TachyonDiagnosticsSources}
    ${TachyonPresetsSources}
    ${TachyonPlatformSources}
    ${TachyonColorSources}
)
tachyon_assert_no_duplicate_sources(GlobalTachyonSources)

add_library(TachyonText STATIC ${TachyonTextSources})
tachyon_configure_common(TachyonText)
tachyon_link_text_deps(TachyonText)

add_library(TachyonEditor STATIC ${TachyonEditorSources})
tachyon_configure_common(TachyonEditor)
target_link_libraries(TachyonEditor PUBLIC TachyonCore)

add_library(TachyonPlatform STATIC ${TachyonPlatformSources})
tachyon_configure_common(TachyonPlatform)

add_library(TachyonLibrary STATIC ${TachyonLibrarySources})
tachyon_configure_common(TachyonLibrary)
target_link_libraries(TachyonLibrary PUBLIC TachyonPresets)

if(TACHYON_ENABLE_TRACKER)
    add_library(TachyonTracker STATIC ${TachyonTrackerSources})
    tachyon_configure_common(TachyonTracker)
endif()

# CLI libraries removed. Sources are added directly to executables to allow different compile-time configurations.

add_library(TachyonCLICore STATIC ${TachyonCLICoreSources} ${TachyonCLILibrarySources})
tachyon_configure_common(TachyonCLICore)
target_link_libraries(TachyonCLICore PUBLIC TachyonRuntimeEngine TachyonLibrary TachyonDiagnostics)

add_executable(tachyon-asset-pack
    tools/tachyon_asset_pack_main.cpp
)
tachyon_configure_common(tachyon-asset-pack)
target_link_libraries(tachyon-asset-pack
    PRIVATE
        TachyonMedia
        TachyonDiagnostics
        TachyonCore
)

add_library(TachyonTools INTERFACE)
target_link_libraries(TachyonTools INTERFACE
    TachyonRuntimeEngine
    TachyonPresets
    TachyonDiagnostics
    TachyonCLICore
)

if(TACHYON_ENABLE_TRACKER)
    target_link_libraries(TachyonTools INTERFACE TachyonTracker)
endif()

# Minimal render-focused executable
add_executable(tachyon
    main.cpp
    ${TachyonCLILibrarySources}
)
tachyon_configure_common(tachyon)

if(TACHYON_ENABLE_PCH AND COMMAND target_precompile_headers)
    target_precompile_headers(tachyon PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/../include/tachyon/pch.h")
endif()

target_link_libraries(tachyon
    PRIVATE
        TachyonCLICore
        TachyonLibrary
        TachyonPresets
        TachyonDiagnostics
)
if(TARGET TachyonRenderer3D)
    target_link_libraries(tachyon PRIVATE TachyonRenderer3D)
endif()

# Full tooling/library executable (optional)
if(TACHYON_BUILD_LIBRARY_TOOLS)
    add_executable(tachyon-library
        main.cpp
        ${TachyonCLICoreSources}
        ${TachyonCLILibrarySources}
    )
    tachyon_configure_common(tachyon-library)
    
    if(TACHYON_ENABLE_PCH AND COMMAND target_precompile_headers)
        target_precompile_headers(tachyon-library PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/../include/tachyon/pch.h")
    endif()

    target_link_libraries(tachyon-library
        PRIVATE
            TachyonRuntimeEngine
            TachyonLibrary
            TachyonEditor
            TachyonPresets
            TachyonDiagnostics
    )
    if(TARGET TachyonRenderer3D)
        target_link_libraries(tachyon-library PRIVATE TachyonRenderer3D)
    endif()
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
