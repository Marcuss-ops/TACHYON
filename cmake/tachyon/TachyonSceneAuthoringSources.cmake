# TachyonScene authoring sources
set(TachyonSceneAuthoringSources
    ${CMAKE_CURRENT_SOURCE_DIR}/core/scene/builder.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/core/scene/data_loader.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/core/scene/dependency_node.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/core/scene/state/evaluated_camera2d_state.cpp

    # Canonical authoring path
    ${CMAKE_CURRENT_SOURCE_DIR}/core/spec/scene_spec_builder.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/core/spec/scene_spec_utils.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/core/spec/cpp_scene_loader.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/core/spec/authoring_service.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/core/spec/asset_resolver.cpp

    ${CMAKE_CURRENT_SOURCE_DIR}/core/analysis/motion_map.cpp
)
