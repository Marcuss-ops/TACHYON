# TachyonMedia sources
set(TachyonMediaSources ${TACHYON_ALL_CPP})
list(FILTER TachyonMediaSources INCLUDE REGEX "/media/[^/]+\\.cpp$|/media/ffmpeg/[^/]+\\.cpp$")
list(FILTER TachyonMediaSources EXCLUDE REGEX "/media/(decoding/svg_decoder_impl|decoding/tinygltf_impl|management/asset_manager|management/image_manager|management/media_manager|loading/mesh_loader|streaming/media_prefetcher|resolution/asset_resolution|import/asset_import_pipeline)\\.cpp$")
list(APPEND TachyonMediaSources
    "${CMAKE_CURRENT_SOURCE_DIR}/media/management/proxy_manifest.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/media/management/proxy_worker.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/media/processing/extruder.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/media/resolution/asset_path_utils.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/media/resolution/asset_resolution.cpp"
)
