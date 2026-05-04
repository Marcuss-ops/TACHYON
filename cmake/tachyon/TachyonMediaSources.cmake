# TachyonMedia sources
set(TachyonMediaSources
    ${CMAKE_CURRENT_SOURCE_DIR}/media/management/asset_manager.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/media/management/image_manager.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/media/management/media_manager.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/media/management/proxy_manifest.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/media/management/proxy_worker.cpp
    
    ${CMAKE_CURRENT_SOURCE_DIR}/media/resolution/asset_resolution.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/media/resolution/asset_path_utils.cpp
    
    ${CMAKE_CURRENT_SOURCE_DIR}/media/processing/extruder.cpp
    
    ${CMAKE_CURRENT_SOURCE_DIR}/media/streaming/media_prefetcher.cpp
    
    ${CMAKE_CURRENT_SOURCE_DIR}/media/loading/mesh_loader.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/media/path_resolver.cpp
    
    ${CMAKE_CURRENT_SOURCE_DIR}/media/import/asset_import_pipeline.cpp
    
    ${CMAKE_CURRENT_SOURCE_DIR}/media/decoding/video_decoder.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/media/decoding/stb_image_impl.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/media/decoding/tinygltf_impl.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/media/decoding/svg_decoder_impl.cpp
)
