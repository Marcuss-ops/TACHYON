cmake_minimum_required(VERSION 3.15)
project(TachyonOperations LANGUAGES CXX)

# --- Sources ---

# --- Sources ---

set(TACHYON_MEDIA_OPS_SOURCES
    src/probe.cpp
    src/probe_cache.cpp
    src/video_concat.cpp
    src/clip_processor.cpp
    src/overlay_merger.cpp
    src/path_resolver.cpp
    src/transition_filters.cpp
    src/pipeline_orchestrator.cpp
)

set(TACHYON_ASSET_OPS_SOURCES
    src/management/asset_manager.cpp
    src/management/image_manager.cpp
    src/management/media_manager.cpp
    src/management/proxy_manifest.cpp
    src/management/proxy_worker.cpp
    src/management/asset_resolver.cpp
    src/resolution/asset_resolution.cpp
    src/resolution/asset_collector.cpp
    src/streaming/media_prefetcher.cpp
    src/import/asset_import_pipeline.cpp
    src/instrumentation.cpp
)

set(TACHYON_DECODING_OPS_SOURCES
    src/decoding/video_decoder.cpp
    src/decoding/svg_decoder_impl.cpp
    src/decoding/stb_image_impl.cpp
)

set(TACHYON_AUDIO_OPS_SOURCES
    src/audio_baker.cpp
    src/presets/sfx/sfx_builders.cpp
    src/presets/sfx/sfx_registry.cpp
    src/presets/sfx/sfx_registry_table.cpp
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

set(TACHYON_COMPRESSION_OPS_SOURCES
    src/compression/texture_compressor_none.cpp
    src/compression/texture_compressor_basis.cpp
    src/compression/compression_simd_kernels_highway.cpp
)

# --- Targets ---

add_library(TachyonMediaOps STATIC ${TACHYON_MEDIA_OPS_SOURCES})
add_library(TachyonAssetOps STATIC ${TACHYON_ASSET_OPS_SOURCES})
add_library(TachyonDecodingOps STATIC ${TACHYON_DECODING_OPS_SOURCES})
add_library(TachyonAudioOps STATIC ${TACHYON_AUDIO_OPS_SOURCES})
add_library(TachyonCompressionOps STATIC ${TACHYON_COMPRESSION_OPS_SOURCES})

# Legacy Interface target for backward compatibility
add_library(TachyonOperations INTERFACE)
target_link_libraries(TachyonOperations INTERFACE 
    TachyonMediaOps 
    TachyonAssetOps 
    TachyonDecodingOps
    TachyonAudioOps 
    TachyonCompressionOps
)

# --- Configuration Helpers ---

macro(setup_ops_target name)
    target_include_directories(${name}
        PUBLIC
            include
            ${CMAKE_SOURCE_DIR}/include
            ${CMAKE_BINARY_DIR}/include
        PRIVATE
            ${CMAKE_SOURCE_DIR}/src
    )
    set_target_properties(${name} PROPERTIES POSITION_INDEPENDENT_CODE ON)
    target_link_libraries(${name} PUBLIC TachyonCore TachyonPlatform pugixml)
endmacro()

setup_ops_target(TachyonMediaOps)
setup_ops_target(TachyonAssetOps)
setup_ops_target(TachyonDecodingOps)
setup_ops_target(TachyonAudioOps)
setup_ops_target(TachyonCompressionOps)

# Specific dependencies
target_link_libraries(TachyonMediaOps PUBLIC TachyonRenderer2D)
target_link_libraries(TachyonAudioOps PUBLIC TachyonPresets)
target_link_libraries(TachyonAssetOps PUBLIC TachyonDecodingOps)

# FFmpeg dependencies
if(TACHYON_ENABLE_MEDIA)
    find_package(PkgConfig QUIET)
    if(PKG_CONFIG_FOUND)
        pkg_check_modules(FFMPEG QUIET libavcodec libavformat libavutil libswscale libswresample)
        if(FFMPEG_FOUND)
            # MediaOps needs FFmpeg for probe and filters
            target_link_libraries(TachyonMediaOps PUBLIC ${FFMPEG_LIBRARIES})
            target_include_directories(TachyonMediaOps PUBLIC ${FFMPEG_INCLUDE_DIRS})
            
            # DecodingOps needs FFmpeg for VideoDecoder
            target_link_libraries(TachyonDecodingOps PUBLIC ${FFMPEG_LIBRARIES})
            target_include_directories(TachyonDecodingOps PUBLIC ${FFMPEG_INCLUDE_DIRS})
            
            # AudioOps needs FFmpeg for resamplers/encoders/decoders
            target_link_libraries(TachyonAudioOps PUBLIC ${FFMPEG_LIBRARIES})
            target_include_directories(TachyonAudioOps PUBLIC ${FFMPEG_INCLUDE_DIRS})
        endif()
    endif()
    target_compile_definitions(TachyonMediaOps PUBLIC TACHYON_ENABLE_MEDIA)
    target_compile_definitions(TachyonDecodingOps PUBLIC TACHYON_ENABLE_MEDIA)
    target_compile_definitions(TachyonAudioOps PUBLIC TACHYON_ENABLE_MEDIA)
endif()

# Highway
if(TACHYON_ENABLE_HIGHWAY)
    set(HWY_TARGET hwy)
    if(TARGET hwy::hwy)
        set(HWY_TARGET hwy::hwy)
    endif()
    
    if(TARGET ${HWY_TARGET})
        target_link_libraries(TachyonCompressionOps PUBLIC ${HWY_TARGET})
    endif()
    target_compile_definitions(TachyonCompressionOps PUBLIC TACHYON_ENABLE_HIGHWAY)
endif()

# Basis
if(TACHYON_ENABLE_BASIS)
    target_compile_definitions(TachyonCompressionOps PUBLIC TACHYON_ENABLE_BASIS)
    target_link_libraries(TachyonCompressionOps PRIVATE basisu_lib)
endif()
