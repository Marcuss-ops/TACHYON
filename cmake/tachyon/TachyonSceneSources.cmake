# TachyonScene sources (Consolidated)

set(TachyonSceneAuthoringSources
    ${CMAKE_CURRENT_SOURCE_DIR}/core/scene/builders/expr_helpers.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/core/scene/builders/anim_helpers.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/core/scene/builders/transition_builder.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/core/scene/builders/material_builder.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/core/scene/builders/camera_builder.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/core/scene/builders/light_builder.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/core/scene/builders/text_builder.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/core/scene/builders/effect_builder.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/core/scene/builders/transform3d_builder.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/core/scene/builders/layer_builder.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/core/scene/builders/composition_builder.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/core/scene/builders/scene_builder.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/core/scene/data_loader.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/core/scene/dependency_node.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/core/scene/state/evaluated_camera2d_state.cpp

    # Canonical authoring path
    ${CMAKE_CURRENT_SOURCE_DIR}/core/spec/scene_spec_builder.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/core/spec/scene_spec_utils.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/core/spec/cpp_scene_loader.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/core/spec/authoring_service.cpp

    ${CMAKE_CURRENT_SOURCE_DIR}/core/analysis/motion_map.cpp
)

set(TachyonSceneSchemaSources
    ${CMAKE_CURRENT_SOURCE_DIR}/core/spec/schema/scene_spec.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/core/spec/schema/scene_spec_core.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/core/spec/schema/migration/scene_migration.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/core/spec/schema/objects/background_spec.cpp
)

set(TachyonSceneValidationSources
    ${CMAKE_CURRENT_SOURCE_DIR}/core/spec/validation/layer_spec_normalizer.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/core/spec/validation/scene_validator_composition.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/core/spec/validation/scene_validator_layer.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/core/spec/validation/scene_validator_schema.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/core/spec/validation/scene_validator_utils.cpp
)

set(TachyonSceneImportSources
    ${CMAKE_CURRENT_SOURCE_DIR}/importer/alembic_importer.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/importer/usd_importer.cpp
)

set(TachyonSceneShapeSources
    ${CMAKE_CURRENT_SOURCE_DIR}/core/spec/shapes/shape_spec_builder.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/core/spec/shapes/shape_factory.cpp
)

set(TachyonSceneSources
    ${TachyonSceneAuthoringSources}
    ${TachyonSceneSchemaSources}
    ${TachyonSceneValidationSources}
    ${TachyonSceneImportSources}
    ${TachyonSceneShapeSources}
)
