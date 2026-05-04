# TachyonOutput sources
set(TachyonOutputSources
    ${CMAKE_CURRENT_SOURCE_DIR}/output/output_utils.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/output/png_sequence_sink.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/core/output/output_presets.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/output/ffmpeg/ffmpeg_command.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/output/ffmpeg/ffmpeg_command_builder.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/output/ffmpeg/ffmpeg_pipe_sink.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/output/ffmpeg/ffmpeg_utils.cpp
)

if(TACHYON_ENABLE_PRORES)
    list(APPEND TachyonOutputSources
        ${CMAKE_CURRENT_SOURCE_DIR}/output/prores_sink.cpp
    )
endif()
