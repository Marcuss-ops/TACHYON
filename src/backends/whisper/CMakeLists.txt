# Tachyon Whisper Backend Module

set(TachyonBackendWhisperSources
    whisper_audio_analyzer.cpp
    register_whisper_backend.cpp
)

add_library(TachyonBackendWhisper STATIC ${TachyonBackendWhisperSources})
tachyon_configure_common(TachyonBackendWhisper)

target_include_directories(TachyonBackendWhisper 
    PUBLIC 
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
)

target_link_libraries(TachyonBackendWhisper
    PUBLIC
        TachyonCore
        TachyonPlatform
)

target_compile_definitions(TachyonBackendWhisper PRIVATE TACHYON_ENABLE_WHISPER)

# Placeholder for whisper specific deps if needed
# target_link_libraries(TachyonBackendWhisper PRIVATE whisper_lib)

set_target_properties(TachyonBackendWhisper PROPERTIES POSITION_INDEPENDENT_CODE ON)
