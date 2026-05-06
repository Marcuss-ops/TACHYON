# TachyonRuntime core sources
set(TachyonRuntimeCoreSources
    ${CMAKE_CURRENT_SOURCE_DIR}/runtime/runtime_facade.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/runtime/core/data/compiled_scene.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/runtime/core/data/property_graph.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/runtime/core/graph/dependency_graph.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/runtime/core/graph/render_graph.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/runtime/core/serialization/tbf_codec.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/runtime/core/serialization/tbf_migration.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/runtime/core/serialization/tbf_read_helpers.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/runtime/core/serialization/tbf_write_helpers.cpp
)
