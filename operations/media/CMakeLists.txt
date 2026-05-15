set(TACHYON_MEDIA_OPS_SOURCES
    src/media_ops.cpp
    src/audio_ops.cpp
)

add_library(TachyonMediaOps STATIC ${TACHYON_MEDIA_OPS_SOURCES})
target_include_directories(TachyonMediaOps
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_BINARY_DIR}/include
)
target_link_libraries(TachyonMediaOps PUBLIC TachyonCore TachyonPlatform)

set_target_properties(TachyonMediaOps PROPERTIES POSITION_INDEPENDENT_CODE ON)
