# Tachyon Decoding Operations (Adapter)

set(TACHYON_DECODING_OPS_SOURCES
    src/decoding_adapter.cpp
)

add_library(TachyonDecodingOps STATIC ${TACHYON_DECODING_OPS_SOURCES})
target_include_directories(TachyonDecodingOps
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_BINARY_DIR}/include
)

target_link_libraries(TachyonDecodingOps 
    PUBLIC 
        TachyonCore 
        TachyonBackendsDecoding
)

set_target_properties(TachyonDecodingOps PROPERTIES POSITION_INDEPENDENT_CODE ON)
