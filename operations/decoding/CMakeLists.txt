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

if(TACHYON_ENABLE_MEDIA)
    find_package(PkgConfig QUIET)
    if(PKG_CONFIG_FOUND)
        pkg_check_modules(FFMPEG QUIET libavcodec libavformat libavutil libswscale libswresample)
        if(FFMPEG_FOUND)
            target_link_libraries(TachyonDecodingOps PUBLIC ${FFMPEG_LIBRARIES})
            target_include_directories(TachyonDecodingOps PUBLIC ${FFMPEG_INCLUDE_DIRS})
        endif()
    endif()
    target_compile_definitions(TachyonDecodingOps PUBLIC TACHYON_ENABLE_MEDIA)
endif()

set_target_properties(TachyonDecodingOps PROPERTIES POSITION_INDEPENDENT_CODE ON)
