cmake_minimum_required(VERSION 3.15)
project(TachyonOperations LANGUAGES CXX)

# --- Sources ---

set(TACHYON_OPERATIONS_SOURCES
    # Core Orchestration
    src/pipeline_orchestrator.cpp
    src/instrumentation.cpp
    
    # Media Operations
    src/probe.cpp
    src/probe_cache.cpp
    src/video_concat.cpp
    src/clip_processor.cpp
    src/overlay_merger.cpp
    src/path_resolver.cpp
    src/transition_filters.cpp
    
    # Management & Resolution
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
    
    # Decoding & Compression
    src/decoding/video_decoder.cpp
    src/decoding/svg_decoder_impl.cpp
    src/decoding/stb_image_impl.cpp
    src/compression/texture_compressor_none.cpp
    src/compression/texture_compressor_basis.cpp
    src/compression/compression_simd_kernels_highway.cpp

    # Audio Operations (Extension)
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

# --- Target ---

add_library(TachyonOperations STATIC ${TACHYON_OPERATIONS_SOURCES})

# --- Configuration ---

target_include_directories(TachyonOperations
    PUBLIC
        include
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_BINARY_DIR}/include
    PRIVATE
        ${CMAKE_SOURCE_DIR}/src
)

target_link_libraries(TachyonOperations
    PUBLIC
        TachyonCore
        TachyonRenderer2D
        TachyonPresets
        TachyonPlatform
        pugixml
)

# FFmpeg dependencies (Essential for Operations)
if(TACHYON_ENABLE_MEDIA)
    find_package(PkgConfig QUIET)
    if(PKG_CONFIG_FOUND)
        pkg_check_modules(FFMPEG QUIET libavcodec libavformat libavutil libswscale libswresample)
        if(FFMPEG_FOUND)
            target_link_libraries(TachyonOperations PUBLIC ${FFMPEG_LIBRARIES})
            target_include_directories(TachyonOperations PUBLIC ${FFMPEG_INCLUDE_DIRS})
        endif()
    endif()
    target_compile_definitions(TachyonOperations PUBLIC TACHYON_ENABLE_MEDIA)
endif()

# Additional dependencies for extended media support
if(TACHYON_ENABLE_DRACO)
    target_compile_definitions(TachyonOperations PUBLIC TACHYON_ENABLE_DRACO)
    target_link_libraries(TachyonOperations PRIVATE draco_static)
endif()

if(TACHYON_ENABLE_BASIS)
    target_compile_definitions(TachyonOperations PUBLIC TACHYON_ENABLE_BASIS)
    target_link_libraries(TachyonOperations PRIVATE basisu_lib)
endif()

# Highway for optimized compression in the operations layer
if(TACHYON_ENABLE_HIGHWAY)
    if(TARGET hwy)
        target_link_libraries(TachyonOperations PUBLIC hwy)
    elseif(TARGET hwy::hwy)
        target_link_libraries(TachyonOperations PUBLIC hwy::hwy)
    endif()
    target_compile_definitions(TachyonOperations PUBLIC TACHYON_ENABLE_HIGHWAY)
endif()

set_target_properties(TachyonOperations PROPERTIES POSITION_INDEPENDENT_CODE ON)
