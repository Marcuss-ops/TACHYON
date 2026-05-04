# Examples CMakeLists.txt
# Builds example programs demonstrating the Tachyon API

# 3D Showcase example - Canonical Modern API Reference
add_executable(3d_showcase authoring/3d_showcase.cpp)
target_link_libraries(3d_showcase PRIVATE TachyonEngine)
set_target_properties(3d_showcase PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/examples"
)
