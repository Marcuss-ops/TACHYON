# Core Engine Library
add_library(TachyonCore src/tachyon.cpp)

target_include_directories(TachyonCore
    PUBLIC 
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../include>
        $<INSTALL_INTERFACE:include>
)

# Placeholder for future source files
# target_sources(TachyonCore PRIVATE core.cpp)

# Link dependencies (none yet)
# target_link_libraries(TachyonCore PRIVATE ...)
