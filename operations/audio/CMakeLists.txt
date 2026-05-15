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

if(TACHYON_ENABLE_MEDIA)
    find_package(PkgConfig QUIET)
    if(PKG_CONFIG_FOUND)
        pkg_check_modules(FFMPEG QUIET libavcodec libavformat libavutil libswscale libswresample)
        if(FFMPEG_FOUND)
            target_link_libraries(TachyonAudioOps PUBLIC ${FFMPEG_LIBRARIES})
            target_include_directories(TachyonAudioOps PUBLIC ${FFMPEG_INCLUDE_DIRS})
        endif()
    endif()
    target_compile_definitions(TachyonAudioOps PUBLIC TACHYON_ENABLE_MEDIA)
endif()

set_target_properties(TachyonAudioOps PROPERTIES POSITION_INDEPENDENT_CODE ON)
