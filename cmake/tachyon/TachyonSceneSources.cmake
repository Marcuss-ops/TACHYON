# TachyonScene sources
set(TachyonSceneSources
    ${CMAKE_CURRENT_SOURCE_DIR}/core/scene/builder.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/core/scene/data_loader.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/core/scene/dependency_node.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/core/scene/state/evaluated_camera2d_state.cpp
    
    # Spec - Core
    ${CMAKE_CURRENT_SOURCE_DIR}/core/spec/scene_spec_builder.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/core/spec/scene_spec_utils.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/core/spec/cpp_scene_loader.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/core/spec/authoring_service.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/core/spec/asset_resolver.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/core/spec/compilation/effect_compiler.cpp
    
    # Spec - Schema
    ${CMAKE_CURRENT_SOURCE_DIR}/core/spec/schema/common/common_spec.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/core/spec/schema/scene_spec.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/core/spec/schema/scene_spec_core.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/core/spec/schema/migration/scene_migration.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/core/spec/schema/objects/background_spec.cpp
    
    # Spec - Validation
    ${CMAKE_CURRENT_SOURCE_DIR}/core/spec/validation/scene_validator_composition.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/core/spec/validation/scene_validator_layer.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/core/spec/validation/scene_validator_schema.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/core/spec/validation/scene_validator_utils.cpp

    # Spec - Shapes
    ${CMAKE_CURRENT_SOURCE_DIR}/core/spec/shapes/shape_spec_builder.cpp
    
    ${CMAKE_CURRENT_SOURCE_DIR}/core/analysis/motion_map.cpp

    # Importers
    ${CMAKE_CURRENT_SOURCE_DIR}/importer/alembic_importer.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/importer/usd_importer.cpp
)
