find_package(OpenGL REQUIRED)

file(GLOB_RECURSE TACHYON_ALL_CPP CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
)

set(TachyonCoreSources ${TACHYON_ALL_CPP})
list(FILTER TachyonCoreSources EXCLUDE REGEX "/(audio|media|output|renderer2d|renderer3d|runtime|text|tracker)/")
list(FILTER TachyonCoreSources EXCLUDE REGEX "/core/(cli|scene|spec)/")
list(FILTER TachyonCoreSources EXCLUDE REGEX "/(background_generator|camera_cut_contract|main)\\.cpp$")

set(TachyonSceneSources ${TACHYON_ALL_CPP})
list(FILTER TachyonSceneSources INCLUDE REGEX "/(core/(scene|spec)|presets|timeline)/|/(background_generator|camera_cut_contract)\\.cpp$")
list(FILTER TachyonSceneSources EXCLUDE REGEX "/core/spec/(serialization|parsing|shapes|schema/migration|template_registry|asset_resolver|pipeline/unified_scene_pipeline)\\.cpp$")

set(TachyonRuntimeSources ${TACHYON_ALL_CPP})
list(FILTER TachyonRuntimeSources INCLUDE REGEX "/runtime/")

set(TachyonRenderer2DSources ${TACHYON_ALL_CPP})
list(FILTER TachyonRenderer2DSources INCLUDE REGEX "/renderer2d/")

set(TachyonRenderer3DSources ${TACHYON_ALL_CPP})
list(FILTER TachyonRenderer3DSources INCLUDE REGEX "/renderer3d/")

set(TachyonMediaSources ${TACHYON_ALL_CPP})
list(FILTER TachyonMediaSources INCLUDE REGEX "/media/[^/]+\\.cpp$|/media/ffmpeg/[^/]+\\.cpp$")
list(FILTER TachyonMediaSources EXCLUDE REGEX "/media/(decoding/svg_decoder_impl|decoding/tinygltf_impl|management/asset_manager|management/image_manager|management/media_manager|loading/mesh_loader|streaming/media_prefetcher|resolution/asset_resolution|import/asset_import_pipeline)\\.cpp$")

set(TachyonAudioSources ${TACHYON_ALL_CPP})
list(FILTER TachyonAudioSources INCLUDE REGEX "/audio/[^/]+\\.cpp$")

set(TachyonOutputSources ${TACHYON_ALL_CPP})
list(FILTER TachyonOutputSources INCLUDE REGEX "/output/[^/]+\\.cpp$|/output/ffmpeg/[^/]+\\.cpp$")
list(FILTER TachyonOutputSources EXCLUDE REGEX "/output/(av_exporter|exr_sequence_sink)\\.cpp$")

set(TachyonTextSources ${TACHYON_ALL_CPP})
list(FILTER TachyonTextSources INCLUDE REGEX "/text/")

set(TachyonTrackerSources ${TACHYON_ALL_CPP})
list(FILTER TachyonTrackerSources INCLUDE REGEX "/tracker/[^/]+\\.cpp$")

set(TachyonCLISources ${TACHYON_ALL_CPP})
list(FILTER TachyonCLISources INCLUDE REGEX "/core/cli/|/core/cli\\.cpp$")

function(tachyon_configure_target target)
    target_include_directories(${target}
        PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/..
        PUBLIC
            $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../include>
            $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../third_party>
            $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../third_party/stb/include>
            $<INSTALL_INTERFACE:include>
    )

    target_link_libraries(${target}
        PUBLIC
            nlohmann_json::nlohmann_json
            freetype
            harfbuzz
            avcodec
            avformat
            avutil
            swscale
            swresample
            embree
            glfw
            OpenGL::GL
            OpenMP::OpenMP_CXX
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

    if(WIN32)
        target_include_directories(${target} PUBLIC $<BUILD_INTERFACE:${oidn_SOURCE_DIR}/include>)
        target_link_libraries(${target} PUBLIC OpenImageDenoise)
    endif()
endfunction()

add_library(TachyonScene OBJECT ${TachyonSceneSources})
tachyon_configure_target(TachyonScene)

add_library(TachyonRuntime OBJECT ${TachyonRuntimeSources})
tachyon_configure_target(TachyonRuntime)

add_library(TachyonRenderer2D OBJECT ${TachyonRenderer2DSources})
tachyon_configure_target(TachyonRenderer2D)

add_library(TachyonRenderer3D OBJECT ${TachyonRenderer3DSources})
tachyon_configure_target(TachyonRenderer3D)

add_library(TachyonMedia OBJECT ${TachyonMediaSources})
tachyon_configure_target(TachyonMedia)

add_library(TachyonAudio OBJECT ${TachyonAudioSources})
tachyon_configure_target(TachyonAudio)

add_library(TachyonOutput OBJECT ${TachyonOutputSources})
tachyon_configure_target(TachyonOutput)

add_library(TachyonText OBJECT ${TachyonTextSources})
tachyon_configure_target(TachyonText)

add_library(TachyonTracker OBJECT ${TachyonTrackerSources})
tachyon_configure_target(TachyonTracker)

add_library(TachyonCLI OBJECT ${TachyonCLISources})
tachyon_configure_target(TachyonCLI)

add_library(TachyonCore STATIC ${TachyonCoreSources})
tachyon_configure_target(TachyonCore)

target_sources(TachyonCore PRIVATE
    $<TARGET_OBJECTS:TachyonScene>
    $<TARGET_OBJECTS:TachyonRuntime>
    $<TARGET_OBJECTS:TachyonRenderer2D>
    $<TARGET_OBJECTS:TachyonRenderer3D>
    $<TARGET_OBJECTS:TachyonMedia>
    $<TARGET_OBJECTS:TachyonAudio>
    $<TARGET_OBJECTS:TachyonOutput>
    $<TARGET_OBJECTS:TachyonText>
    $<TARGET_OBJECTS:TachyonTracker>
    $<TARGET_OBJECTS:TachyonCLI>
)

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
    add_custom_command(TARGET tachyon POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
                ${OIDN_RUNTIME_DLLS}
                $<TARGET_FILE_DIR:tachyon>
        COMMAND_EXPAND_LISTS
    )
endif()
