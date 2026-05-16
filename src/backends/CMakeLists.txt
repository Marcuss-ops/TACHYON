# Tachyon Backends Module

if(TACHYON_ENABLE_MEDIA)
    add_subdirectory(ffmpeg)
endif()

if(TACHYON_ENABLE_WHISPER)
    add_subdirectory(whisper)
endif()

add_subdirectory(compression)
add_subdirectory(decoding)

if(TACHYON_ENABLE_HIGHWAY OR TACHYON_ENABLE_MANUAL_AVX2)
    add_subdirectory(simd)
endif()

add_subdirectory(registry)

# TachyonBackends is now an interface aggregator to decouple the engine
# from concrete implementation details.
add_library(TachyonBackends INTERFACE)

target_link_libraries(TachyonBackends
    INTERFACE
        TachyonCore
        TachyonPlatform
        TachyonBackendsCompression
        TachyonBackendsDecoding
        TachyonBackendRegistry
)

if(TACHYON_ENABLE_MEDIA)
    target_link_libraries(TachyonBackends INTERFACE TachyonBackendFfmpeg)
endif()

if(TACHYON_ENABLE_WHISPER)
    target_link_libraries(TachyonBackends INTERFACE TachyonBackendWhisper)
endif()

if(TACHYON_ENABLE_HIGHWAY OR TACHYON_ENABLE_MANUAL_AVX2)
    target_link_libraries(TachyonBackends INTERFACE TachyonBackendSimd)
endif()