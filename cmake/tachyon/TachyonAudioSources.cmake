# TachyonAudio sources
set(TachyonAudioSources
    ${CMAKE_CURRENT_SOURCE_DIR}/audio/core/audio_mixer.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/audio/core/audio_graph.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/audio/core/sound_effect_registry.cpp
    
    ${CMAKE_CURRENT_SOURCE_DIR}/audio/processing/audio_analyzer.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/audio/processing/audio_processor.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/audio/processing/loudness_meter.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/audio/processing/waveform.cpp
    
    ${CMAKE_CURRENT_SOURCE_DIR}/audio/io/audio_decoder.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/audio/io/audio_encoder.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/audio/io/audio_export.cpp
)
