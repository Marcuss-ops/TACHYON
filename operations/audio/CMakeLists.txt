set(TACHYON_AUDIO_OPS_SOURCES
    src/audio_baker.cpp
    src/audio_filter_builder.cpp
    src/audio_resampler.cpp
    src/gate_utils.cpp
    src/audio/core/audio_graph.cpp
    src/audio/core/audio_mixer.cpp
    src/audio/processing/waveform.cpp
    src/audio/processing/audio_processor.cpp
    src/audio/processing/loudness_meter.cpp
    src/audio/processing/audio_analyzer.cpp
    src/audio/io/audio_export.cpp
    src/audio/io/audio_encoder.cpp
    src/audio/io/audio_decoder.cpp
)

add_library(TachyonAudioOps STATIC ${TACHYON_AUDIO_OPS_SOURCES})
target_include_directories(TachyonAudioOps
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_BINARY_DIR}/include
)
target_link_libraries(TachyonAudioOps PUBLIC TachyonCore TachyonPlatform TachyonPresets pugixml)

tachyon_link_media_deps(TachyonAudioOps)

set_target_properties(TachyonAudioOps PROPERTIES POSITION_INDEPENDENT_CODE ON)
