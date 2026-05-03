if(TACHYON_ENABLE_PREVIEW_WINDOW)
    find_package(OpenGL REQUIRED)
endif()

include(${CMAKE_SOURCE_DIR}/cmake/tachyon/TachyonCoreSources.cmake)
include(${CMAKE_SOURCE_DIR}/cmake/tachyon/TachyonPlatformSources.cmake)
message(STATUS "DEBUG: include path: ${CMAKE_SOURCE_DIR}/cmake/tachyon/TachyonSceneSources.cmake")
include(${CMAKE_SOURCE_DIR}/cmake/tachyon/TachyonSceneSources.cmake)
include(${CMAKE_SOURCE_DIR}/cmake/tachyon/TachyonRuntimeSources.cmake)
include(${CMAKE_SOURCE_DIR}/cmake/tachyon/TachyonRenderer2DSources.cmake)
include(${CMAKE_SOURCE_DIR}/cmake/tachyon/TachyonRenderer3DSources.cmake)
include(${CMAKE_SOURCE_DIR}/cmake/tachyon/TachyonMediaSources.cmake)
include(${CMAKE_SOURCE_DIR}/cmake/tachyon/TachyonAudioSources.cmake)
include(${CMAKE_SOURCE_DIR}/cmake/tachyon/TachyonOutputSources.cmake)
include(${CMAKE_SOURCE_DIR}/cmake/tachyon/TachyonTextSources.cmake)
include(${CMAKE_SOURCE_DIR}/cmake/tachyon/TachyonCLISources.cmake)


function(tachyon_deduplicate_sources var_name)
    set(_list "${${var_name}}")
    list(REMOVE_DUPLICATES _list)
    set(${var_name} "${_list}" PARENT_SCOPE)
endfunction()

# tachyon_deduplicate_sources(TachyonSceneSources)
# tachyon_deduplicate_sources(TachyonRuntimeSources)
# tachyon_deduplicate_sources(TachyonRenderer2DSources)
# tachyon_deduplicate_sources(TachyonRenderer3DSources)
# tachyon_deduplicate_sources(TachyonMediaSources)
# tachyon_deduplicate_sources(TachyonAudioSources)
# tachyon_deduplicate_sources(TachyonOutputSources)
# tachyon_deduplicate_sources(TachyonTextSources)
if(TACHYON_ENABLE_TRACKER)
    include(${CMAKE_SOURCE_DIR}/cmake/tachyon/TachyonTrackerSources.cmake)
    tachyon_deduplicate_sources(TachyonTrackerSources)
endif()
tachyon_deduplicate_sources(TachyonCLISources)

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

function(tachyon_configure_target target)
    tachyon_configure_common(${target})
    tachyon_link_text_deps(${target})
    tachyon_link_media_deps(${target})
    tachyon_link_3d_deps(${target})
    tachyon_link_omp(${target})
    tachyon_link_output_deps(${target})
endfunction()

add_library(TachyonScene OBJECT ${TachyonSceneSources})
tachyon_configure_common(TachyonScene)

add_library(TachyonRuntime OBJECT ${TachyonRuntimeSources})
tachyon_configure_common(TachyonRuntime)
if(TACHYON_ENABLE_AUDIO_MUX)
    target_compile_definitions(TachyonRuntime PRIVATE TACHYON_ENABLE_AUDIO_MUX)
endif()
tachyon_link_3d_deps(TachyonRuntime)
tachyon_link_omp(TachyonRuntime)

add_library(TachyonRenderer2D OBJECT ${TachyonRenderer2DSources})
tachyon_configure_common(TachyonRenderer2D)
tachyon_link_text_deps(TachyonRenderer2D)
tachyon_link_3d_deps(TachyonRenderer2D)
tachyon_link_omp(TachyonRenderer2D)

add_library(TachyonRenderer3D OBJECT ${TachyonRenderer3DSources})
tachyon_configure_common(TachyonRenderer3D)
tachyon_link_3d_deps(TachyonRenderer3D)
tachyon_link_omp(TachyonRenderer3D)

add_library(TachyonMedia OBJECT ${TachyonMediaSources})
tachyon_configure_common(TachyonMedia)
tachyon_link_media_deps(TachyonMedia)
target_link_libraries(TachyonMedia PUBLIC pugixml)

add_library(TachyonAudio OBJECT ${TachyonAudioSources})
tachyon_configure_common(TachyonAudio)

add_library(TachyonOutput OBJECT ${TachyonOutputSources})
tachyon_configure_common(TachyonOutput)
tachyon_link_output_deps(TachyonOutput)

add_library(TachyonText OBJECT ${TachyonTextSources})
tachyon_configure_common(TachyonText)
tachyon_link_text_deps(TachyonText)

if(TACHYON_ENABLE_TRACKER)
add_library(TachyonTracker STATIC ${TachyonTrackerSources})
tachyon_configure_common(TachyonTracker)
endif()

add_library(TachyonCLI OBJECT ${TachyonCLISources})
tachyon_configure_common(TachyonCLI)
tachyon_link_3d_deps(TachyonCLI)

add_library(TachyonPlatform OBJECT ${TachyonPlatformSources})
tachyon_configure_common(TachyonPlatform)

add_library(TachyonCore STATIC 
    ${TachyonCoreSources}
    ${TachyonSceneSources}
    ${TachyonRuntimeSources}
    ${TachyonRenderer2DSources}
    ${TachyonRenderer3DSources}
    ${TachyonMediaSources}
    ${TachyonAudioSources}
    ${TachyonOutputSources}
    ${TachyonTextSources}
    ${TachyonCLISources}
    ${TachyonPlatformSources}
)
tachyon_configure_common(TachyonCore)
if(TACHYON_ENABLE_AUDIO_MUX)
    target_compile_definitions(TachyonCore PUBLIC TACHYON_ENABLE_AUDIO_MUX)
endif()
tachyon_link_text_deps(TachyonCore)
tachyon_link_media_deps(TachyonCore)
tachyon_link_3d_deps(TachyonCore)
tachyon_link_omp(TachyonCore)
tachyon_link_output_deps(TachyonCore)
if(TACHYON_ENABLE_TRACKER)
    target_link_libraries(TachyonCore PUBLIC TachyonTracker)
endif()
if(TACHYON_ENABLE_PREVIEW_WINDOW)
    target_compile_definitions(TachyonCore PUBLIC TACHYON_ENABLE_PREVIEW_WINDOW)
endif()

add_executable(tachyon
    main.cpp
)

if(TACHYON_ENABLE_PCH AND COMMAND target_precompile_headers)
    target_precompile_headers(tachyon PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/../include/tachyon/pch.h")
endif()

target_link_libraries(tachyon
    PRIVATE
        TachyonCore
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
