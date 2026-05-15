set(TACHYON_DECODING_OPS_SOURCES
    src/video_decoder.cpp
    src/svg_decoder_impl.cpp
    src/stb_image_impl.cpp
)

add_library(TachyonDecodingOps STATIC ${TACHYON_DECODING_OPS_SOURCES})
target_include_directories(TachyonDecodingOps
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_BINARY_DIR}/include
)
target_link_libraries(TachyonDecodingOps PUBLIC TachyonCore TachyonPlatform pugixml)

tachyon_link_media_deps(TachyonDecodingOps)

set_target_properties(TachyonDecodingOps PROPERTIES POSITION_INDEPENDENT_CODE ON)
