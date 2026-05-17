# Tachyon Output Module

set(TachyonOutputSources
    output_utils.cpp
    png_sequence_sink.cpp
    ../core/output/output_presets.cpp
    ffmpeg/ffmpeg_command.cpp
    ffmpeg/ffmpeg_command_builder.cpp
    ffmpeg/ffmpeg_capabilities.cpp
    ffmpeg/ffmpeg_pipe_sink.cpp
    output_planner.cpp
    ffmpeg/ffmpeg_utils.cpp
    async_output_sink.cpp
)

if(TACHYON_ENABLE_PRORES)
    list(APPEND TachyonOutputSources prores_sink.cpp)
endif()

if(TACHYON_ENABLE_OUTPUT)
    add_library(TachyonOutput STATIC ${TachyonOutputSources})
    tachyon_configure_common(TachyonOutput)
    target_include_directories(TachyonOutput PUBLIC 
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    )
    target_include_directories(TachyonOutput PRIVATE ${tinyexr_SOURCE_DIR})
    tachyon_link_media_deps(TachyonOutput)
    tachyon_link_output_deps(TachyonOutput)
    target_link_libraries(TachyonOutput PUBLIC TachyonColor TachyonPlatform)
endif()
