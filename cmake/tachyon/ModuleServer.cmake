# TachyonServer module
set(TachyonServerSources
    ${CMAKE_CURRENT_SOURCE_DIR}/server/server.cpp
)

add_library(TachyonServer STATIC ${TachyonServerSources})
tachyon_configure_common(TachyonServer)
target_link_libraries(TachyonServer PUBLIC TachyonRuntime)
