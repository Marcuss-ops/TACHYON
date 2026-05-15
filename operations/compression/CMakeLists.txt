# Tachyon Compression Operations (Adapter)

set(TACHYON_COMPRESSION_OPS_SOURCES
    src/compression_adapter.cpp
)

add_library(TachyonCompressionOps STATIC ${TACHYON_COMPRESSION_OPS_SOURCES})
target_include_directories(TachyonCompressionOps
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_BINARY_DIR}/include
)

target_link_libraries(TachyonCompressionOps 
    PUBLIC 
        TachyonCore 
        TachyonBackendsCompression
)

set_target_properties(TachyonCompressionOps PROPERTIES POSITION_INDEPENDENT_CODE ON)
