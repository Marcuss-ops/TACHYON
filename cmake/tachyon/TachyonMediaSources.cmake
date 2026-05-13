# TachyonMedia sources
set(TachyonMediaSources
    ${CMAKE_CURRENT_SOURCE_DIR}/media/management/asset_manager.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/media/management/image_manager.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/media/management/media_manager.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/media/management/proxy_manifest.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/media/management/proxy_worker.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/media/management/asset_resolver.cpp
    
    ${CMAKE_CURRENT_SOURCE_DIR}/media/resolution/asset_resolution.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/media/streaming/media_prefetcher.cpp
    
    ${CMAKE_CURRENT_SOURCE_DIR}/media/path_resolver.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/media/probe.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/media/probe_cache.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/media/video_concat.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/media/overlay_merger.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/media/clip_processor.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/media/transition_filters.cpp
    
    ${CMAKE_CURRENT_SOURCE_DIR}/media/import/asset_import_pipeline.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/media/decoding/video_decoder.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/media/decoding/svg_decoder_impl.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/media/decoding/stb_image_impl.cpp

    ${CMAKE_CURRENT_SOURCE_DIR}/media/compression/texture_compressor_none.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/media/compression/texture_compressor_basis.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/media/compression/compression_simd_kernels_highway.cpp
)
