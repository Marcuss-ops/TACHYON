# Tachyon Backends Module

add_subdirectory(ffmpeg)
add_subdirectory(whisper)
add_subdirectory(compression)
add_subdirectory(decoding)
add_subdirectory(simd)
add_subdirectory(registry)

# TachyonBackends is now an interface aggregator to decouple the engine
# from concrete implementation details.
add_library(TachyonBackends INTERFACE)

target_link_libraries(TachyonBackends
    INTERFACE
        TachyonCore
        TachyonPlatform
        TachyonBackendSimd
        TachyonBackendsCompression
        TachyonBackendsDecoding
        TachyonBackendFfmpeg
        TachyonBackendWhisper
        TachyonBackendRegistry
)