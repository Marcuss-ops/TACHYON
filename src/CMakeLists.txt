if(TACHYON_ENABLE_PREVIEW_WINDOW)
    find_package(OpenGL REQUIRED)
endif()

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
include(${CMAKE_SOURCE_DIR}/cmake/tachyon/TachyonCLISources.cmake)
include(${CMAKE_SOURCE_DIR}/cmake/tachyon/TachyonStudioSources.cmake)


function(tachyon_deduplicate_sources var_name)
    set(_list "${${var_name}}")
    list(REMOVE_DUPLICATES _list)
    set(${var_name} "${_list}" PARENT_SCOPE)
endfunction()

tachyon_deduplicate_sources(TachyonSceneSources)
tachyon_deduplicate_sources(TachyonRuntimeSources)
tachyon_deduplicate_sources(TachyonRenderer2DSources)
tachyon_deduplicate_sources(TachyonRenderer3DSources)
tachyon_deduplicate_sources(TachyonMediaSources)
tachyon_deduplicate_sources(TachyonAudioSources)
tachyon_deduplicate_sources(TachyonOutputSources)
tachyon_deduplicate_sources(TachyonTextSources)
tachyon_deduplicate_sources(TachyonCLISources)
tachyon_deduplicate_sources(TachyonStudioSources)

if(TACHYON_ENABLE_TRACKER)
    include(${CMAKE_SOURCE_DIR}/cmake/tachyon/TachyonTrackerSources.cmake)
    tachyon_deduplicate_sources(TachyonTrackerSources)
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
            $<BUILD_INTERFACE:${tinygltf_SOURCE_DIR}>
            $<BUILD_INTERFACE:${tinyexr_SOURCE_DIR}>
            $<BUILD_INTERFACE:${pugixml_SOURCE_DIR}/src>
            $<BUILD_INTERFACE:${TACHYON_EARCUT_INCLUDE_DIR}>
            $<BUILD_INTERFACE:${clipper2_SOURCE_DIR}/CPP/Clipper2Lib/include>
            $<INSTALL_INTERFACE:include>
    )

    if(MSVC)
        target_compile_options(${target} PRIVATE /W4 /permissive- /utf-8 /FS /arch:AVX2)
        if(TACHYON_TREAT_WARNINGS_AS_ERRORS)
            target_compile_options(${target} PRIVATE /WX)
        endif()
        target_compile_definitions(${target} PRIVATE TACHYON_AVX2)
    else()
        target_compile_options(${target} PRIVATE -Wall -Wextra -Wpedantic)
        if(TACHYON_TREAT_WARNINGS_AS_ERRORS)
            target_compile_options(${target} PRIVATE -Werror)
        endif()
    endif()

    target_compile_definitions(${target} PRIVATE "TACHYON_SFX_ROOT=\"${CMAKE_CURRENT_SOURCE_DIR}/audio/SoundEffect\"")
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
    target_link_libraries(${target} PUBLIC OpenMP::OpenMP_CXX)
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

add_library(TachyonScene STATIC ${TachyonSceneSources})
tachyon_configure_common(TachyonScene)

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

add_library(TachyonRenderer3D STATIC ${TachyonRenderer3DSources})
tachyon_configure_common(TachyonRenderer3D)
tachyon_link_3d_deps(TachyonRenderer3D)
tachyon_link_omp(TachyonRenderer3D)

add_library(TachyonMedia STATIC ${TachyonMediaSources})
tachyon_configure_common(TachyonMedia)
tachyon_link_media_deps(TachyonMedia)
target_link_libraries(TachyonMedia PUBLIC pugixml)

add_library(TachyonAudio STATIC ${TachyonAudioSources})
tachyon_configure_common(TachyonAudio)
tachyon_link_media_deps(TachyonAudio)

add_library(TachyonOutput STATIC ${TachyonOutputSources})
tachyon_configure_common(TachyonOutput)
tachyon_link_media_deps(TachyonOutput)
tachyon_link_output_deps(TachyonOutput)

add_library(TachyonText STATIC ${TachyonTextSources})
tachyon_configure_common(TachyonText)
tachyon_link_text_deps(TachyonText)

add_library(TachyonCLI STATIC ${TachyonCLISources})
tachyon_configure_common(TachyonCLI)
tachyon_link_3d_deps(TachyonCLI)
tachyon_link_omp(TachyonCLI)
tachyon_link_output_deps(TachyonCLI)

add_library(TachyonPlatform STATIC ${TachyonPlatformSources})
tachyon_configure_common(TachyonPlatform)

add_library(TachyonStudio STATIC ${TachyonStudioSources})
tachyon_configure_common(TachyonStudio)

if(TACHYON_ENABLE_TRACKER)
    add_library(TachyonTracker STATIC ${TachyonTrackerSources})
    tachyon_configure_common(TachyonTracker)
endif()

add_library(TachyonEngine INTERFACE)
target_link_libraries(TachyonEngine INTERFACE
    TachyonCore
    TachyonScene
    TachyonRuntime
    TachyonRenderer2D
    TachyonRenderer3D
    TachyonMedia
    TachyonAudio
    TachyonOutput
    TachyonText
    TachyonCLI
    TachyonPlatform
    TachyonStudio
)

if(TACHYON_ENABLE_TRACKER)
    target_link_libraries(TachyonEngine INTERFACE TachyonTracker)
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
        TachyonEngine
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
