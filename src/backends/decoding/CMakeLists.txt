set(TACHYON_BACKEND_DECODING_SOURCES
    ffmpeg_video_decoder.cpp
    svg_decoder.cpp
    stb_image_impl.cpp
    decoding_factory.cpp
)

add_library(TachyonBackendsDecoding STATIC ${TACHYON_BACKEND_DECODING_SOURCES})

target_include_directories(TachyonBackendsDecoding
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../../../include>
        ${CMAKE_BINARY_DIR}/include
)

target_link_libraries(TachyonBackendsDecoding 
    PUBLIC 
        TachyonCore 
        TachyonPlatform
        pugixml
)

# FFmpeg integration
tachyon_link_media_deps(TachyonBackendsDecoding)

set_target_properties(TachyonBackendsDecoding PROPERTIES POSITION_INDEPENDENT_CODE ON)
