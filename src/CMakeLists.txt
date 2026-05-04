if(TACHYON_ENABLE_PREVIEW_WINDOW)
    find_package(OpenGL REQUIRED)
endif()

include(${CMAKE_SOURCE_DIR}/cmake/tachyon/TachyonPresetsSources.cmake)
include(${CMAKE_SOURCE_DIR}/cmake/tachyon/TachyonCoreSources.cmake)
include(${CMAKE_SOURCE_DIR}/cmake/tachyon/TachyonPlatformSources.cmake)
include(${CMAKE_SOURCE_DIR}/cmake/tachyon/TachyonSceneSources.cmake)
include(${CMAKE_SOURCE_DIR}/cmake/tachyon/TachyonRuntimeSources.cmake)
include(${CMAKE_SOURCE_DIR}/cmake/tachyon/TachyonRenderer2DSources.cmake)
include(${CMAKE_SOURCE_DIR}/cmake/tachyon/TachyonRenderer3DSources.cmake)
include(${CMAKE_SOURCE_DIR}/cmake/tachyon/TachyonMediaSources.cmake)
include(${CMAKE_SOURCE_DIR}/cmake/tachyon/TachyonAudioSources.cmake)
include(${CMAKE_SOURCE_DIR}/cmake/tachyon/TachyonOutputSources.cmake)
include(${CMAKE_SOURCE_DIR}/cmake/tachyon/TachyonTextSources.cmake)
include(${CMAKE_SOURCE_DIR}/cmake/tachyon/TachyonEditorSources.cmake)
include(${CMAKE_SOURCE_DIR}/cmake/tachyon/TachyonCLISources.cmake)
include(${CMAKE_SOURCE_DIR}/cmake/tachyon/TachyonStudioSources.cmake)
include(${CMAKE_SOURCE_DIR}/cmake/tachyon/TachyonColorSources.cmake)


function(tachyon_assert_no_duplicate_sources var_name)
    set(_list "${${var_name}}")
    set(_list_copy "${_list}")
    list(REMOVE_DUPLICATES _list_copy)
    list(LENGTH _list _len1)
    list(LENGTH _list_copy _len2)
    if(NOT _len1 EQUAL _len2)
        message(FATAL_ERROR "Duplicate sources found in ${var_name}! Please fix the source lists in cmake/tachyon/.")
    endif()
endfunction()

tachyon_assert_no_duplicate_sources(TachyonSceneSources)
tachyon_assert_no_duplicate_sources(TachyonRuntimeSources)
tachyon_assert_no_duplicate_sources(TachyonRenderer2DSources)
tachyon_assert_no_duplicate_sources(TachyonRenderer3DSources)
tachyon_assert_no_duplicate_sources(TachyonMediaSources)
tachyon_assert_no_duplicate_sources(TachyonAudioSources)
tachyon_assert_no_duplicate_sources(TachyonOutputSources)
tachyon_assert_no_duplicate_sources(TachyonTextSources)
tachyon_assert_no_duplicate_sources(TachyonEditorSources)
tachyon_assert_no_duplicate_sources(TachyonCLISources)
tachyon_assert_no_duplicate_sources(TachyonStudioSources)

if(TACHYON_ENABLE_TRACKER)
    include(${CMAKE_SOURCE_DIR}/cmake/tachyon/TachyonTrackerSources.cmake)
    tachyon_assert_no_duplicate_sources(TachyonTrackerSources)
endif()

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

    # Use AssetResolver instead of compile-time roots.
    set_target_properties(${target} PROPERTIES POSITION_INDEPENDENT_CODE ON)
endfunction()

function(tachyon_link_text_deps target)
    target_link_libraries(${target} PUBLIC freetype harfbuzz)
endfunction()

function(tachyon_link_media_deps target)
    target_link_libraries(${target} PUBLIC avcodec avformat avutil swscale swresample)
endfunction()

function(tachyon_link_3d_deps target)
    target_link_libraries(${target} PUBLIC embree)
    if(TACHYON_ENABLE_OIDN AND WIN32)
        target_include_directories(${target} PUBLIC $<BUILD_INTERFACE:${oidn_SOURCE_DIR}/include>)
        target_link_libraries(${target} PUBLIC OpenImageDenoise)
    endif()
endfunction()

function(tachyon_link_omp target)
    if(TACHYON_HAS_OPENMP AND TARGET OpenMP::OpenMP_CXX)
        target_link_libraries(${target} PUBLIC OpenMP::OpenMP_CXX)
    endif()
endfunction()

function(tachyon_link_output_deps target)
    if(TACHYON_ENABLE_PREVIEW_WINDOW)
        target_link_libraries(${target} PUBLIC glfw OpenGL::GL)
        target_compile_definitions(${target} PUBLIC TACHYON_ENABLE_PREVIEW_WINDOW)
    endif()

    if(TACHYON_ENABLE_OIDN AND WIN32)
        target_include_directories(${target} PUBLIC $<BUILD_INTERFACE:${oidn_SOURCE_DIR}/include>)
        target_link_libraries(${target} PUBLIC OpenImageDenoise)
    endif()
endfunction()

add_library(TachyonCore STATIC ${TachyonCoreSources})
tachyon_configure_common(TachyonCore)
target_include_directories(TachyonCore
    PUBLIC
        $<BUILD_INTERFACE:${clipper2_SOURCE_DIR}/CPP/Clipper2Lib/include>
)

add_library(TachyonColor STATIC ${TachyonColorSources})
tachyon_configure_common(TachyonColor)
target_link_libraries(TachyonColor PUBLIC TachyonCore)

if(MSVC)
    set_source_files_properties(color/blend_kernel_avx2.cpp PROPERTIES COMPILE_FLAGS "/arch:AVX2")
    target_compile_definitions(TachyonColor PRIVATE "TACHYON_AVX2")
endif()

add_library(TachyonScene STATIC ${TachyonSceneSources})
tachyon_configure_common(TachyonScene)
if(TACHYON_ENABLE_TRACKER)
    target_link_libraries(TachyonScene PUBLIC TachyonTracker)
endif()

add_library(TachyonRuntime STATIC ${TachyonRuntimeSources})
tachyon_configure_common(TachyonRuntime)
if(TACHYON_ENABLE_AUDIO_MUX)
    target_compile_definitions(TachyonRuntime PRIVATE TACHYON_ENABLE_AUDIO_MUX)
endif()
tachyon_link_3d_deps(TachyonRuntime)
tachyon_link_omp(TachyonRuntime)

add_library(TachyonRenderer2D STATIC ${TachyonRenderer2DSources})
tachyon_configure_common(TachyonRenderer2D)
tachyon_link_text_deps(TachyonRenderer2D)
tachyon_link_3d_deps(TachyonRenderer2D)
tachyon_link_omp(TachyonRenderer2D)
target_link_libraries(TachyonRenderer2D PUBLIC TachyonColor)

add_library(TachyonRenderer3D STATIC ${TachyonRenderer3DSources})
tachyon_configure_common(TachyonRenderer3D)
tachyon_link_3d_deps(TachyonRenderer3D)
tachyon_link_omp(TachyonRenderer3D)
target_link_libraries(TachyonRenderer3D PUBLIC TachyonColor)

add_library(TachyonMedia STATIC ${TachyonMediaSources})
tachyon_configure_common(TachyonMedia)
target_include_directories(TachyonMedia
    PRIVATE
        ${tinygltf_SOURCE_DIR}
        ${pugixml_SOURCE_DIR}/src
        ${TACHYON_EARCUT_INCLUDE_DIR}
)
tachyon_link_media_deps(TachyonMedia)
target_link_libraries(TachyonMedia PUBLIC pugixml)

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

add_library(TachyonText STATIC ${TachyonTextSources})
tachyon_configure_common(TachyonText)
tachyon_link_text_deps(TachyonText)

add_library(TachyonEditor STATIC ${TachyonEditorSources})
tachyon_configure_common(TachyonEditor)

add_library(TachyonPlatform STATIC ${TachyonPlatformSources})
tachyon_configure_common(TachyonPlatform)

add_library(TachyonStudio STATIC ${TachyonStudioSources})
tachyon_configure_common(TachyonStudio)

if(TACHYON_ENABLE_TRACKER)
    add_library(TachyonTracker STATIC ${TachyonTrackerSources})
    tachyon_configure_common(TachyonTracker)
endif()

add_library(TachyonRuntimeEngine INTERFACE)
target_link_libraries(TachyonRuntimeEngine INTERFACE
    TachyonCore
    TachyonScene
    TachyonRuntime
    TachyonRenderer2D
    TachyonRenderer3D
    TachyonMedia
    TachyonAudio
    TachyonOutput
    TachyonText
    TachyonPlatform
)

add_library(TachyonEngine INTERFACE)
target_link_libraries(TachyonEngine INTERFACE TachyonRuntimeEngine)

add_library(TachyonCLI STATIC ${TachyonCLISources})
tachyon_configure_common(TachyonCLI)
target_link_libraries(TachyonCLI PUBLIC TachyonRuntimeEngine TachyonStudio TachyonEditor)
if(TACHYON_ENABLE_PREVIEW_WINDOW)
    target_link_libraries(TachyonCLI PUBLIC glfw OpenGL::GL)
    target_compile_definitions(TachyonCLI PUBLIC TACHYON_ENABLE_PREVIEW_WINDOW)
endif()

add_library(TachyonTools INTERFACE)
target_link_libraries(TachyonTools INTERFACE
    TachyonCLI
    TachyonStudio
)

if(TACHYON_ENABLE_TRACKER)
    target_link_libraries(TachyonTools INTERFACE TachyonTracker)
endif()

add_executable(tachyon
    main.cpp
    core/cli.cpp
)

if(TACHYON_ENABLE_PCH AND COMMAND target_precompile_headers)
    target_precompile_headers(tachyon PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/../include/tachyon/pch.h")
endif()

target_link_libraries(tachyon
    PRIVATE
        TachyonRuntimeEngine
        TachyonCLI
)

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
