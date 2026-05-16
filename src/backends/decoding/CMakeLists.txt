# Tachyon Image Decoding (STB)
add_library(TachyonBackendImageDecoding STATIC stb_image_impl.cpp)
target_include_directories(TachyonBackendImageDecoding PUBLIC 
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../../../include>
    ${CMAKE_BINARY_DIR}/include
)
target_link_libraries(TachyonBackendImageDecoding PUBLIC TachyonCore TachyonPlatform)
set_target_properties(TachyonBackendImageDecoding PROPERTIES POSITION_INDEPENDENT_CODE ON)

# Tachyon SVG Decoding (pugixml)
add_library(TachyonBackendSvgDecoding STATIC svg_decoder.cpp)
target_include_directories(TachyonBackendSvgDecoding PUBLIC 
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../../../include>
    ${CMAKE_BINARY_DIR}/include
)
target_link_libraries(TachyonBackendSvgDecoding PUBLIC TachyonCore TachyonPlatform pugixml)
set_target_properties(TachyonBackendSvgDecoding PROPERTIES POSITION_INDEPENDENT_CODE ON)

# Tachyon FFmpeg Video Decoding
if(TACHYON_ENABLE_MEDIA)
    add_library(TachyonBackendFfmpegVideoDecoding STATIC ffmpeg_video_decoder.cpp)
    target_include_directories(TachyonBackendFfmpegVideoDecoding PUBLIC 
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../../../include>
        ${CMAKE_BINARY_DIR}/include
    )
    target_link_libraries(TachyonBackendFfmpegVideoDecoding PUBLIC TachyonCore TachyonPlatform)
    target_compile_definitions(TachyonBackendFfmpegVideoDecoding PRIVATE TACHYON_ENABLE_MEDIA)
    tachyon_link_media_deps(TachyonBackendFfmpegVideoDecoding)
    set_target_properties(TachyonBackendFfmpegVideoDecoding PROPERTIES POSITION_INDEPENDENT_CODE ON)
endif()

# Main Decoding Aggregator
add_library(TachyonBackendsDecoding STATIC decoding_factory.cpp)
target_include_directories(TachyonBackendsDecoding PUBLIC 
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../../../include>
    ${CMAKE_BINARY_DIR}/include
)
target_link_libraries(TachyonBackendsDecoding PUBLIC 
    TachyonBackendImageDecoding 
    TachyonBackendSvgDecoding
)
if(TACHYON_ENABLE_MEDIA)
    target_link_libraries(TachyonBackendsDecoding PUBLIC TachyonBackendFfmpegVideoDecoding)
endif()
set_target_properties(TachyonBackendsDecoding PROPERTIES POSITION_INDEPENDENT_CODE ON)
