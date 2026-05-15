set(TACHYON_ASSET_OPS_SOURCES
    src/asset_manager.cpp
    src/image_manager.cpp
    src/media_manager.cpp
    src/proxy_manifest.cpp
    src/proxy_worker.cpp
    src/asset_resolver.cpp
    src/media_prefetcher.cpp
    src/asset_import_pipeline.cpp
    src/instrumentation.cpp
    src/path_resolver.cpp

)

add_library(TachyonAssetOps STATIC ${TACHYON_ASSET_OPS_SOURCES})
target_include_directories(TachyonAssetOps
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_BINARY_DIR}/include
)
target_link_libraries(TachyonAssetOps 
    PUBLIC 
        TachyonCore 
        TachyonPlatform 
        TachyonDecodingOps 
        TachyonAudioOps
        pugixml
)
set_target_properties(TachyonAssetOps PROPERTIES POSITION_INDEPENDENT_CODE ON)
