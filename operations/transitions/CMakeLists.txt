set(TACHYON_TRANSITION_OPS_SOURCES
    src/transition_ops.cpp
)

add_library(TachyonTransitionOps STATIC ${TACHYON_TRANSITION_OPS_SOURCES})

target_include_directories(TachyonTransitionOps
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(TachyonTransitionOps
    PUBLIC
        TachyonCore
        TachyonRenderer2D
)

set_target_properties(TachyonTransitionOps PROPERTIES POSITION_INDEPENDENT_CODE ON)
